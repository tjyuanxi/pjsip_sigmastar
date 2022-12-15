/* $Id$ */
/*
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <pjmedia-videodev/videodev_imp.h>
#include <pjmedia/event.h>
#include <pj/assert.h>
#include <pj/log.h>
#include <pj/os.h>
#include "jpeglib.h"

#if defined(PJMEDIA_HAS_VIDEO) && PJMEDIA_HAS_VIDEO != 0 && \
    defined(PJMEDIA_VIDEO_DEV_HAS_SIGMASTAR) && PJMEDIA_VIDEO_DEV_HAS_SIGMASTAR != 0

#include "mi_common.h"
#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_disp_datatype.h"
#include "mi_panel_datatype.h"
#include "mi_sys_datatype.h"
#include "libyuv/convert_from.h"
#include "SAT070CP50_1024x600.h"
#include "libyuv/scale.h"

#define THIS_FILE		"sigmstar_dev.c"
#define DEFAULT_CLOCK_RATE	90000
#define DEFAULT_WIDTH		640
#define DEFAULT_HEIGHT		480
#define DEFAULT_FPS		25

#define MAKE_YUYV_VALUE(y,u,v) ((y) << 24) | ((u) << 16) | ((y) << 8) | (v)
#define YUYV_BLACK MAKE_YUYV_VALUE(0,128,128)
#define YUYV_WHITE MAKE_YUYV_VALUE(255,128,128)
#define YUYV_RED MAKE_YUYV_VALUE(76,84,255)
#define YUYV_GREEN MAKE_YUYV_VALUE(149,43,21)
#define YUYV_BLUE MAKE_YUYV_VALUE(29,225,107)
#define ALIGN_UP(x, a)            (((x+a-1) / (a)) * (a))


static int ijpg = 0;
typedef struct ST_Sys_Rect_s
{
    MI_S32 s32X;
    MI_S32 s32Y;
    MI_U16 u16PicW;
    MI_U16 u16PicH;
} ST_Rect_t;

typedef struct sigmastar_fmt_info
{
    pjmedia_format_id   fmt_id;
    MI_U32              sigmastar_format;
    MI_U32              Rmask;
    MI_U32              Gmask;
    MI_U32              Bmask;
    MI_U32              Amask;
} sigmastar_fmt_info;

static sigmastar_fmt_info sigmastar_fmts[] =
{
    {PJMEDIA_FORMAT_I420, 1448433993, 0, 0, 0, 0}
    // {PJMEDIA_FORMAT_NV12, 842094158, 0, 0, 0, 0} 

};

/* sigmastar_ device info */
struct sigmastar_dev_info
{
    pjmedia_vid_dev_info	 info;
};

/* Linked list of streams */
struct stream_list
{
    PJ_DECL_LIST_MEMBER(struct stream_list);
    struct sigmastar_stream	*stream;
};

#define INITIAL_MAX_JOBS 64
#define JOB_QUEUE_INC_FACTOR 2

typedef pj_status_t (*job_func_ptr)(void *data);

typedef struct job {
    job_func_ptr    func;
    void           *data;
    unsigned        flags;
    pj_status_t     retval;
} job;

typedef struct job_queue {
    pj_pool_t      *pool;
    job           **jobs;
    pj_sem_t      **job_sem;
    pj_sem_t      **old_sem;
    pj_mutex_t     *mutex;
    pj_thread_t    *thread;
    pj_sem_t       *sem;

    unsigned        size;
    unsigned        head, tail;
    pj_bool_t	    is_full;
    pj_bool_t       is_quitting;
} job_queue;

/* sigmastar_ factory */
struct sigmastar_factory
{
    pjmedia_vid_dev_factory	 base;
    pj_pool_t			*pool;
    pj_pool_factory		*pf;

    unsigned			 dev_count;
    struct sigmastar_dev_info	        *dev_info;
    job_queue                   *jq;

    pj_thread_t			*sigmastar_thread;        /**< sigmastar thread.        */
    pj_sem_t                    *sem;
    pj_mutex_t			*mutex;
    struct stream_list		 streams;
    pj_bool_t                    is_quitting;
    pj_thread_desc 		 thread_desc;
    pj_thread_t 		*ev_thread;
};

/* Video stream. */
struct sigmastar_stream
{
    pjmedia_vid_dev_stream	 base;		    /**< Base stream	    */
    pjmedia_vid_dev_param	 param;		    /**< Settings	    */
    pj_pool_t			*pool;              /**< Memory pool.       */

    pjmedia_vid_dev_cb		 vid_cb;            /**< Stream callback.   */
    void			*user_data;         /**< Application data.  */

    struct sigmastar_factory          *sf;
    const pjmedia_frame         *frame;
    pj_bool_t			 is_running;
    pj_timestamp		 last_ts;
    struct stream_list		 list_entry;

    int                          pitch;             /**< Pitch value.       */
    ST_Rect_t			 rect;              /**< Frame rectangle.   */
    ST_Rect_t			 dstrect;           /**< Display rectangle. */

    pjmedia_video_apply_fmt_param vafp;
};

/* Prototypes */
static pj_status_t sigmastar_factory_init(pjmedia_vid_dev_factory *f);
static pj_status_t sigmastar_factory_destroy(pjmedia_vid_dev_factory *f);
static pj_status_t sigmastar_factory_refresh(pjmedia_vid_dev_factory *f);
static unsigned    sigmastar_factory_get_dev_count(pjmedia_vid_dev_factory *f);
static pj_status_t sigmastar_factory_get_dev_info(pjmedia_vid_dev_factory *f,
					    unsigned index,
					    pjmedia_vid_dev_info *info);
static pj_status_t sigmastar_factory_default_param(pj_pool_t *pool,
                                             pjmedia_vid_dev_factory *f,
					     unsigned index,
					     pjmedia_vid_dev_param *param);
static pj_status_t sigmastar_factory_create_stream(
					pjmedia_vid_dev_factory *f,
					pjmedia_vid_dev_param *param,
					const pjmedia_vid_dev_cb *cb,
					void *user_data,
					pjmedia_vid_dev_stream **p_vid_strm);

static pj_status_t sigmastar_stream_get_param(pjmedia_vid_dev_stream *strm,
					pjmedia_vid_dev_param *param);
static pj_status_t sigmastar_stream_get_cap(pjmedia_vid_dev_stream *strm,
				      pjmedia_vid_dev_cap cap,
				      void *value);
static pj_status_t sigmastar_stream_set_cap(pjmedia_vid_dev_stream *strm,
				      pjmedia_vid_dev_cap cap,
				      const void *value);
static pj_status_t sigmastar_stream_put_frame(pjmedia_vid_dev_stream *strm,
                                        const pjmedia_frame *frame);
static pj_status_t sigmastar_stream_start(pjmedia_vid_dev_stream *strm);
static pj_status_t sigmastar_stream_stop(pjmedia_vid_dev_stream *strm);
static pj_status_t sigmastar_stream_destroy(pjmedia_vid_dev_stream *strm);

static pj_status_t resize_disp(struct sigmastar_stream *strm,
                               pjmedia_rect_size *new_disp_size);
static pj_status_t sigmastar_destroy_all(void *data);

/* Job queue prototypes */
static pj_status_t job_queue_create(pj_pool_t *pool, job_queue **pjq);
static pj_status_t job_queue_post_job(job_queue *jq, job_func_ptr func,
				      void *data, unsigned flags,
				      pj_status_t *retval);
static pj_status_t job_queue_destroy(job_queue *jq);

static pj_status_t sigmastar_quit(void *data);

/* Operations */
static pjmedia_vid_dev_factory_op factory_op =
{
    &sigmastar_factory_init,
    &sigmastar_factory_destroy,
    &sigmastar_factory_get_dev_count,
    &sigmastar_factory_get_dev_info,
    &sigmastar_factory_default_param,
    &sigmastar_factory_create_stream,
    &sigmastar_factory_refresh
};

static pjmedia_vid_dev_stream_op stream_op =
{
    &sigmastar_stream_get_param,
    &sigmastar_stream_get_cap,
    &sigmastar_stream_set_cap,
    &sigmastar_stream_start,
    NULL,
    &sigmastar_stream_put_frame,
    &sigmastar_stream_stop,
    &sigmastar_stream_destroy
};

/*
 * Util
 */
static void sigmastar_log_err(const char *op, MI_S32 ret)
{
    PJ_LOG(1,(THIS_FILE, "%s error: %d", op, ret));
}

/****************************************************************************
 * Factory operations
 */
/*
 * Init sigmastar video driver.
 */
pjmedia_vid_dev_factory* pjmedia_sigmastar_factory(pj_pool_factory *pf)
{
    struct sigmastar_factory *f;
    pj_pool_t *pool;

    pool = pj_pool_create(pf, "sigmastar video", 1000, 1000, NULL);
    f = PJ_POOL_ZALLOC_T(pool, struct sigmastar_factory);
    f->pf = pf;
    f->pool = pool;
    f->base.op = &factory_op;

    return &f->base;
}


int sstar_disp_init(MI_DISP_PubAttr_t *pstDispPubAttr)
{
    MI_PANEL_LinkType_e eLinkType = E_MI_PNL_LINK_TTL;

    MI_SYS_Init();

    if (pstDispPubAttr->eIntfType == E_MI_DISP_INTF_LCD)
    {
        pstDispPubAttr->stSyncInfo.u16Vact = stPanelParam.u16Height;
        pstDispPubAttr->stSyncInfo.u16Vbb = stPanelParam.u16VSyncBackPorch;
        pstDispPubAttr->stSyncInfo.u16Vfb = stPanelParam.u16VTotal - (stPanelParam.u16VSyncWidth +
                                                                      stPanelParam.u16Height + stPanelParam.u16VSyncBackPorch);
        pstDispPubAttr->stSyncInfo.u16Hact = stPanelParam.u16Width;
        pstDispPubAttr->stSyncInfo.u16Hbb = stPanelParam.u16HSyncBackPorch;
        pstDispPubAttr->stSyncInfo.u16Hfb = stPanelParam.u16HTotal - (stPanelParam.u16HSyncWidth +
                                                                      stPanelParam.u16Width + stPanelParam.u16HSyncBackPorch);
        pstDispPubAttr->stSyncInfo.u16Bvact = 0;
        pstDispPubAttr->stSyncInfo.u16Bvbb = 0;
        pstDispPubAttr->stSyncInfo.u16Bvfb = 0;
        pstDispPubAttr->stSyncInfo.u16Hpw = stPanelParam.u16HSyncWidth;
        pstDispPubAttr->stSyncInfo.u16Vpw = stPanelParam.u16VSyncWidth;
        pstDispPubAttr->stSyncInfo.u32FrameRate = stPanelParam.u16DCLK * 1000000 / (stPanelParam.u16HTotal * stPanelParam.u16VTotal);
        pstDispPubAttr->eIntfSync = E_MI_DISP_OUTPUT_USER;
        pstDispPubAttr->eIntfType = E_MI_DISP_INTF_LCD;
        eLinkType = stPanelParam.eLinkType;
    }

    MI_DISP_SetPubAttr(0, pstDispPubAttr);
    MI_DISP_Enable(0);
    MI_DISP_BindVideoLayer(0, 0);
    // MI_DISP_EnableVideoLayer(0);

    // MI_DISP_EnableInputPort(0, 0);
    // MI_DISP_SetInputPortSyncMode(0, 0, E_MI_DISP_SYNC_MODE_FREE_RUN);

    if (pstDispPubAttr->eIntfType == E_MI_DISP_INTF_LCD)
    {
        MI_S32 ret = MI_PANEL_Init(eLinkType);
        if (ret != MI_SUCCESS)
        {
            PJ_LOG(1, (THIS_FILE, "MI_PANEL_Init:%d fialed %d", ret, __LINE__));
            return 0;
        }
        ret = MI_PANEL_SetPanelParam(&stPanelParam);
        PJ_LOG(4, (THIS_FILE, "MI_PANEL_SetPanelParam:%d , %d", ret, __LINE__));
        if (eLinkType == E_MI_PNL_LINK_MIPI_DSI)
        {
            MI_PANEL_SetMipiDsiConfig(&stMipiDsiConfig);
        }
    }
    return 0;
}
int sdk_Init(void)
{
    MI_DISP_PubAttr_t stDispPubAttr = {0};

    stDispPubAttr.eIntfType = E_MI_DISP_INTF_LCD;
    stDispPubAttr.eIntfSync = E_MI_DISP_OUTPUT_USER;
    stDispPubAttr.u32BgColor = YUYV_BLACK;
    // sstar_disp_init(&stDispPubAttr);

    return 0;
}

static pj_status_t sigmastar_init(void * data)
{
    PJ_UNUSED_ARG(data);
    // sdk_Init();
    return PJ_SUCCESS;
}

// static struct sigmastar_stream* find_stream(struct sigmastar_factory *sf,
//                                       MI_U32 windowID,
//                                       pjmedia_event *pevent)
// {
//     struct stream_list *it, *itBegin;
//     struct sigmastar_stream *strm = NULL;

//     itBegin = &sf->streams;
//     for (it = itBegin->next; it != itBegin; it = it->next) {
//         if (SDL_GetWindowID(it->stream->window) == windowID)
//         {
//             strm = it->stream;
//             break;
//         }
//     }
 
//     if (strm)
//         pjmedia_event_init(pevent, PJMEDIA_EVENT_NONE, &strm->last_ts,
// 		           strm);

//     return strm;
// }

static pj_status_t handle_event(void *data)
{
    return PJ_SUCCESS;
}

static int sigmastar_ev_thread(void *data)
{
    struct sigmastar_factory *sf = (struct sigmastar_factory*)data;

    while(1) {
        pj_status_t status;

        pj_mutex_lock(sf->mutex);
        if (pj_list_empty(&sf->streams)) {
            pj_mutex_unlock(sf->mutex);
            /* Wait until there is any stream. */
            pj_sem_wait(sf->sem);
        } else
            pj_mutex_unlock(sf->mutex);

        if (sf->is_quitting)
            break;

        job_queue_post_job(sf->jq, handle_event, sf, 0, &status);

        pj_thread_sleep(50);
    }

    return 0;
}


/* API: init factory */
static pj_status_t sigmastar_factory_init(pjmedia_vid_dev_factory *f)
{
    struct sigmastar_factory *sf = (struct sigmastar_factory*)f;
    struct sigmastar_dev_info *ddi;
    unsigned i, j;
    pj_status_t status;

    pj_list_init(&sf->streams);

    status = job_queue_create(sf->pool, &sf->jq);
    if (status != PJ_SUCCESS)
        return PJMEDIA_EVID_INIT;

    job_queue_post_job(sf->jq, sigmastar_init, NULL, 0, &status);
    if (status != PJ_SUCCESS)
        return status;

    status = pj_mutex_create_recursive(sf->pool, "sigmastar_factory",
				       &sf->mutex);
    if (status != PJ_SUCCESS)
	return status;

    status = pj_sem_create(sf->pool, NULL, 0, 1, &sf->sem);
    if (status != PJ_SUCCESS)
	return status;

    /* Create event handler thread. */
    status = pj_thread_create(sf->pool, "sigmastar_thread", sigmastar_ev_thread,
			      sf, 0, 0, &sf->sigmastar_thread);
    if (status != PJ_SUCCESS)
        return status;

    sf->dev_count = 1;

    sf->dev_info = (struct sigmastar_dev_info*)
		   pj_pool_calloc(sf->pool, sf->dev_count,
				  sizeof(struct sigmastar_dev_info));

    ddi = &sf->dev_info[0];
    pj_bzero(ddi, sizeof(*ddi));
    strncpy(ddi->info.name, "sigmastar renderer", sizeof(ddi->info.name));
    ddi->info.name[sizeof(ddi->info.name)-1] = '\0';
    ddi->info.fmt_cnt = PJ_ARRAY_SIZE(sigmastar_fmts);

    for (i = 0; i < sf->dev_count; i++) {
        ddi = &sf->dev_info[i];
        strncpy(ddi->info.driver, "sigmastar", sizeof(ddi->info.driver));
        ddi->info.driver[sizeof(ddi->info.driver)-1] = '\0';
        ddi->info.dir = PJMEDIA_DIR_RENDER;
        ddi->info.has_callback = PJ_FALSE;
        ddi->info.caps = PJMEDIA_VID_DEV_CAP_FORMAT |
                         PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE;
        ddi->info.caps |= PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW;
        ddi->info.caps |= PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW_FLAGS;

        for (j = 0; j < ddi->info.fmt_cnt; j++) {
            pjmedia_format *fmt = &ddi->info.fmt[j];
            pjmedia_format_init_video(fmt, sigmastar_fmts[j].fmt_id,
                                      DEFAULT_WIDTH, DEFAULT_HEIGHT,
                                      DEFAULT_FPS, 1);
        }
    }
    return PJ_SUCCESS;
}

/* API: destroy factory */
static pj_status_t sigmastar_factory_destroy(pjmedia_vid_dev_factory *f)
{
    struct sigmastar_factory *sf = (struct sigmastar_factory*)f;
    pj_pool_t *pool = sf->pool;
    pj_status_t status;

    pj_assert(pj_list_empty(&sf->streams));

    sf->is_quitting = PJ_TRUE;
    if (sf->sigmastar_thread) {
        pj_sem_post(sf->sem);
        pj_thread_join(sf->sigmastar_thread);
        pj_thread_destroy(sf->sigmastar_thread);
    }

    if (sf->mutex) {
	pj_mutex_destroy(sf->mutex);
	sf->mutex = NULL;
    }

    if (sf->sem) {
        pj_sem_destroy(sf->sem);
        sf->sem = NULL;
    }

    job_queue_post_job(sf->jq, sigmastar_quit, NULL, 0, &status);
    job_queue_destroy(sf->jq);

    sf->pool = NULL;
    pj_pool_release(pool);

    return PJ_SUCCESS;
}

/* API: refresh the list of devices */
static pj_status_t sigmastar_factory_refresh(pjmedia_vid_dev_factory *f)
{
    PJ_UNUSED_ARG(f);
    return PJ_SUCCESS;
}

/* API: get number of devices */
static unsigned sigmastar_factory_get_dev_count(pjmedia_vid_dev_factory *f)
{
    struct sigmastar_factory *sf = (struct sigmastar_factory*)f;
    return sf->dev_count;
}

/* API: get device info */
static pj_status_t sigmastar_factory_get_dev_info(pjmedia_vid_dev_factory *f,
					    unsigned index,
					    pjmedia_vid_dev_info *info)
{
    struct sigmastar_factory *sf = (struct sigmastar_factory*)f;

    PJ_ASSERT_RETURN(index < sf->dev_count, PJMEDIA_EVID_INVDEV);

    pj_memcpy(info, &sf->dev_info[index].info, sizeof(*info));

    return PJ_SUCCESS;
}

/* API: create default device parameter */
static pj_status_t sigmastar_factory_default_param(pj_pool_t *pool,
                                             pjmedia_vid_dev_factory *f,
					     unsigned index,
					     pjmedia_vid_dev_param *param)
{
    struct sigmastar_factory *sf = (struct sigmastar_factory*)f;
    struct sigmastar_dev_info *di = &sf->dev_info[index];

    PJ_ASSERT_RETURN(index < sf->dev_count, PJMEDIA_EVID_INVDEV);
    
    PJ_UNUSED_ARG(pool);

    pj_bzero(param, sizeof(*param));
    param->dir = PJMEDIA_DIR_RENDER;
    param->rend_id = index;
    param->cap_id = PJMEDIA_VID_INVALID_DEV;

    /* Set the device capabilities here */
    param->flags = PJMEDIA_VID_DEV_CAP_FORMAT;
    param->fmt.type = PJMEDIA_TYPE_VIDEO;
    param->clock_rate = DEFAULT_CLOCK_RATE;
    pj_memcpy(&param->fmt, &di->info.fmt[0], sizeof(param->fmt));

    return PJ_SUCCESS;
}

static sigmastar_fmt_info* get_sigmastar_format_info(pjmedia_format_id id)
{
    unsigned i;

    for (i = 0; i < sizeof(sigmastar_fmts)/sizeof(sigmastar_fmts[0]); i++) {
        if (sigmastar_fmts[i].fmt_id == id)
            return &sigmastar_fmts[i];
    }

    return NULL;
}
int sstar_disp_Deinit(MI_DISP_PubAttr_t *pstDispPubAttr)
{
    // MI_DISP_ClearInputPortBuffer(0, 0, true);
    MI_DISP_DisableInputPort(0, 0);
    MI_DISP_DisableVideoLayer(0);
    MI_DISP_UnBindVideoLayer(0, 0);
    MI_DISP_Disable(0);

    printf("sstar_disp_Deinit...\n");

    return 0;
}


int sdk_DeInit(void)
{
    //unbind modules
    MI_DISP_PubAttr_t stDispPubAttr = {0};
    stDispPubAttr.eIntfType = E_MI_DISP_INTF_LCD;
    stDispPubAttr.eIntfSync = E_MI_DISP_OUTPUT_USER;
    stDispPubAttr.u32BgColor = YUYV_BLACK;
    sstar_disp_Deinit(&stDispPubAttr);

    return 0;
}


static pj_status_t sigmastar_quit(void *data)
{
    PJ_UNUSED_ARG(data);
    // SDL_Quit();
    sdk_DeInit();
    return PJ_SUCCESS;
}

// static pj_status_t put_frame(void *data);

static pj_status_t sigmastar_destroy(void *data)
{
    MI_DISP_ClearInputPortBuffer(0, 0, true);
    // MI_DISP_DisableInputPort(0, 0);
    // MI_DISP_DisableVideoLayer(0);
    // sdk_DeInit();
    return PJ_SUCCESS;
}

static pj_status_t sigmastar_destroy_all(void *data)
{
    struct sigmastar_stream *strm = (struct sigmastar_stream *)data;  
    sigmastar_destroy(data);
    return PJ_SUCCESS;
}

static pj_status_t sigmastar_create_window(struct sigmastar_stream *strm, 
				     pj_bool_t use_app_win,
				     MI_U32 sigmastar_format,
				     pjmedia_vid_dev_hwnd *hwnd)
{

    return PJ_SUCCESS;
}

static pj_status_t sigmastar_create_rend(struct sigmastar_stream * strm,
                                   pjmedia_format *fmt)
{
    sigmastar_fmt_info *sigmastar_info;
    const pjmedia_video_format_info *vfi;
    pjmedia_video_format_detail *vfd;

    sigmastar_info = get_sigmastar_format_info(fmt->id);
    vfi = pjmedia_get_video_format_info(pjmedia_video_format_mgr_instance(),
                                        fmt->id);
    if (!vfi || !sigmastar_info)
        return PJMEDIA_EVID_BADFORMAT;

    strm->vafp.size = fmt->det.vid.size;
    strm->vafp.buffer = NULL;
    if (vfi->apply_fmt(vfi, &strm->vafp) != PJ_SUCCESS)
        return PJMEDIA_EVID_BADFORMAT;

    vfd = pjmedia_format_get_video_format_detail(fmt, PJ_TRUE);
    strm->rect.s32X = strm->rect.s32Y = 0;
    strm->rect.u16PicW = (MI_U16)vfd->size.w;
    strm->rect.u16PicH = (MI_U16)vfd->size.h;
    if (strm->param.disp_size.w == 0)
        strm->param.disp_size.w = strm->rect.u16PicW;
    if (strm->param.disp_size.h == 0)
        strm->param.disp_size.h = strm->rect.u16PicH;

    if (strm->dstrect.u16PicH == 0) {
        strm->dstrect.u16PicH = strm->rect.u16PicH;
    }
    if (strm->dstrect.u16PicW == 0) {
        strm->dstrect.u16PicW = strm->rect.u16PicW;
    }

    // strm->dstrect.s32X = strm->dstrect.s32Y = 0;
    // strm->dstrect.u16PicW = (MI_U16)strm->param.disp_size.w;
    // strm->dstrect.u16PicH = (MI_U16)strm->param.disp_size.h;

    // MI_DISP_InputPortAttr_t stInputPortAttr;
    // memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));
    // MI_S32 ret = MI_DISP_GetInputPortAttr(0, 0, &stInputPortAttr);
    // // stInputPortAttr.u16SrcWidth = input_width;
    // // stInputPortAttr.u16SrcHeight = input_height;
    // stInputPortAttr.stDispWin.u16Height = strm->dstrect.u16PicH;
    // stInputPortAttr.stDispWin.u16Width = strm->dstrect.u16PicW;
    // stInputPortAttr.stDispWin.u16X = strm->dstrect.s32X;
    // stInputPortAttr.stDispWin.u16Y = strm->dstrect.s32Y;
    // stInputPortAttr.u16SrcHeight = strm->rect.u16PicH;
    // stInputPortAttr.u16SrcWidth = strm->rect.u16PicW;
    // ret = MI_DISP_SetInputPortAttr(0, 0, &stInputPortAttr);

    return PJ_SUCCESS;
}

static pj_status_t sigmastar_create(void *data)
{
    struct sigmastar_stream *strm = (struct sigmastar_stream *)data;

    MI_DISP_EnableVideoLayer(0);
    MI_DISP_EnableInputPort(0, 0);
    MI_DISP_SetInputPortSyncMode(0, 0, E_MI_DISP_SYNC_MODE_FREE_RUN);
    return sigmastar_create_rend(strm, &strm->param.fmt);
}


static pj_status_t reposition_disp(struct sigmastar_stream *strm,
                               int x, int y)
{
    strm->param.window_pos.x = x;
    strm->param.window_pos.y = y;
    
    // strm->dstrect.s32X = 0;
    // strm->dstrect.s32Y = 0;
    strm->dstrect.s32X = x;
    strm->dstrect.s32Y = y;
    MI_DISP_InputPortAttr_t stInputPortAttr;
    memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));
    MI_S32 ret = MI_DISP_GetInputPortAttr(0, 0, &stInputPortAttr);

    stInputPortAttr.stDispWin.u16Width = strm->dstrect.u16PicW;
    stInputPortAttr.stDispWin.u16Height = strm->dstrect.u16PicH;
    stInputPortAttr.stDispWin.u16X = strm->dstrect.s32X;
    stInputPortAttr.stDispWin.u16Y = strm->dstrect.s32Y;

    ret = MI_DISP_SetInputPortAttr(0, 0, &stInputPortAttr);
    PJ_LOG(4, (THIS_FILE, "reposition_disp:x:%d, y:%d, h:%d, w:%d, sH:%d, sW:%d, %d", stInputPortAttr.stDispWin.u16X, 
        stInputPortAttr.stDispWin.u16Y, stInputPortAttr.stDispWin.u16Height, stInputPortAttr.stDispWin.u16Width, 
        stInputPortAttr.u16SrcHeight, stInputPortAttr.u16SrcWidth, __LINE__));
    if (ret != MI_SUCCESS) {
        PJ_LOG(4, (THIS_FILE, "reposition_disp failed, x:%d, y:%d, w:%d, h:%d, ret:%d", 
        strm->dstrect.s32X, strm->dstrect.s32Y, strm->dstrect.u16PicW, strm->dstrect.u16PicH, ret));
        return PJ_EINVALIDOP;
    }
    return PJ_SUCCESS;
}

static pj_status_t resize_disp(struct sigmastar_stream *strm,
                               pjmedia_rect_size *new_disp_size)
{
    pj_memcpy(&strm->param.disp_size, new_disp_size,
              sizeof(strm->param.disp_size));
    
    // strm->dstrect.s32X = 0;
    // strm->dstrect.s32Y = 0;
    strm->dstrect.u16PicW = (MI_U16)strm->param.disp_size.w;
    strm->dstrect.u16PicH = (MI_U16)strm->param.disp_size.h;
    MI_DISP_InputPortAttr_t stInputPortAttr;
    memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));

    MI_S32 ret = MI_DISP_GetInputPortAttr(0, 0, &stInputPortAttr);

    stInputPortAttr.stDispWin.u16Width = strm->dstrect.u16PicW;
    stInputPortAttr.stDispWin.u16Height = strm->dstrect.u16PicH;
    stInputPortAttr.stDispWin.u16X = strm->dstrect.s32X;
    stInputPortAttr.stDispWin.u16Y = strm->dstrect.s32Y;
    stInputPortAttr.u16SrcHeight = strm->dstrect.u16PicH;
    stInputPortAttr.u16SrcWidth = strm->dstrect.u16PicW;
    ret = MI_DISP_SetInputPortAttr(0, 0, &stInputPortAttr);

    PJ_LOG(4, (THIS_FILE, "resize_disp:x:%d, y:%d, h:%d, w:%d, sH:%d, sW:%d, %d", stInputPortAttr.stDispWin.u16X, 
        stInputPortAttr.stDispWin.u16Y, stInputPortAttr.stDispWin.u16Height, stInputPortAttr.stDispWin.u16Width, 
        stInputPortAttr.u16SrcHeight, stInputPortAttr.u16SrcWidth, __LINE__));
    if (ret != MI_SUCCESS) {
        PJ_LOG(4, (THIS_FILE, "resize_disp failed, x:%d, y:%d, w:%d, h:%d, ret:%d", 
        strm->dstrect.s32X, strm->dstrect.s32Y, strm->dstrect.u16PicW, strm->dstrect.u16PicH, ret));
        return PJ_EINVALIDOP;
    }
    return PJ_SUCCESS;
}

static pj_status_t change_format(struct sigmastar_stream *strm,
                                 pjmedia_format *new_fmt)
{
    pj_status_t status;

    /* Recreate sigmastar renderer */
    PJ_LOG(4, (THIS_FILE, "change_format:%d, %d, %d", new_fmt->id, new_fmt->det.vid.size.h, new_fmt->det.vid.size.w));
    status = sigmastar_create_rend(strm, (new_fmt? new_fmt :
				   &strm->param.fmt));
    if (status == PJ_SUCCESS && new_fmt)
        pjmedia_format_copy(&strm->param.fmt, new_fmt);

    return status;
}


void ST_I420ToNV12(const uint8_t* pu8src_y,
                       int src_stride_y,
                       const uint8_t* pu8src_u,
                       int src_stride_u,
                       const uint8_t* pu8src_v,
                       int src_stride_v,
                       uint8_t* pu8dst_y,
                       int dst_stride_y,
                       uint8_t* pu8dst_uv,
                       int dst_stride_uv,
                       int width,
                       int height)
{
    const uint8_t* pu8stsrc_y = pu8src_y;
    const uint8_t* pu8stsrc_u = pu8src_u;
    const uint8_t* pu8stsrc_v = pu8src_v;
    uint8_t* pu8stdst_y = pu8dst_y;
    uint8_t* pu8stdst_uv = pu8dst_uv;

    I420ToNV12(pu8stsrc_y,src_stride_y,pu8stsrc_u,src_stride_u,pu8stsrc_v,src_stride_v,pu8stdst_y,dst_stride_y,pu8stdst_uv,dst_stride_uv,width,height);
}

int _mi_ChnInputPortPutBuf(MI_SYS_BUF_HANDLE bufHandle, ST_Rect_t* frame_rect, MI_SYS_BufInfo_t* stBufInfo, pjmedia_frame * frame)
{
    unsigned char* pu8STSrcU = NULL;
    unsigned char* pu8STSrcV = NULL;
    int u32Index = 0;
    char *tmp_yuv_buf = NULL;
    char *tmp_frame_buf = NULL;
    stBufInfo->stFrameData.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stBufInfo->stFrameData.eFieldType = E_MI_SYS_FIELDTYPE_NONE;
    stBufInfo->stFrameData.eTileMode = E_MI_SYS_FRAME_TILE_MODE_NONE;
    stBufInfo->bEndOfStream = 0;
    tmp_frame_buf = (char *)calloc(frame_rect->u16PicW *frame_rect->u16PicH *3/2, 1);
    if(tmp_frame_buf == NULL)
    {
        printf("malloc tmp_frame_buf fail \n");
        return -1;
    }
    char *tmp_uv_buf = tmp_frame_buf + (frame_rect->u16PicW * frame_rect->u16PicH);


    pu8STSrcU = (unsigned char*)frame->buf + frame_rect->u16PicW * frame_rect->u16PicH;
    pu8STSrcV = (unsigned char*)frame->buf + frame_rect->u16PicW * frame_rect->u16PicH + frame_rect->u16PicW * frame_rect->u16PicH/2/2;

    //判断是否为绿色
    if (0 == memcmp(tmp_frame_buf, frame->buf, (frame_rect->u16PicW * frame_rect->u16PicH * 2 / 3) - 5))
    {
        memset(pu8STSrcU, 128, frame_rect->u16PicW * frame_rect->u16PicH / 2);
    }

    ST_I420ToNV12((unsigned char*)frame->buf, frame_rect->u16PicW,
                    pu8STSrcU, frame_rect->u16PicW/2,
                    pu8STSrcV, frame_rect->u16PicW/2,
                    (unsigned char*)tmp_frame_buf, frame_rect->u16PicW,
                    (unsigned char*)tmp_uv_buf, frame_rect->u16PicW,
                    frame_rect->u16PicW, frame_rect->u16PicH);

    //dump_file("/customer/origin_img",tmp_frame_buf,pImage->width*pImage->height*3/2);

    tmp_yuv_buf = tmp_frame_buf;

    for (u32Index = 0; u32Index < stBufInfo->stFrameData.u16Height; u32Index ++)
    {
        memcpy(stBufInfo->stFrameData.pVirAddr[0]+(u32Index*stBufInfo->stFrameData.u32Stride[0]),
                    tmp_yuv_buf+(u32Index*stBufInfo->stFrameData.u16Width), stBufInfo->stFrameData.u16Width);
    }
    for (u32Index = 0; u32Index < stBufInfo->stFrameData.u16Height / 2; u32Index ++)
    {
        memcpy(stBufInfo->stFrameData.pVirAddr[1]+(u32Index*stBufInfo->stFrameData.u32Stride[1]),
                    tmp_yuv_buf+(u32Index*stBufInfo->stFrameData.u16Width)+(frame_rect->u16PicW * frame_rect->u16PicH),stBufInfo->stFrameData.u16Width);
    }

    stBufInfo->stFrameData.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;


    if(MI_SUCCESS != MI_SYS_ChnInputPortPutBuf(bufHandle, stBufInfo, 0))
    {
        free(tmp_frame_buf);
        printf("MI_SYS_ChnInputPortPutBuf fail \n");
        return -1;
    }
    free(tmp_frame_buf);
    return 0;
}



int _mi_ChnInputPortGetBuf(MI_SYS_BUF_HANDLE* bufHandle, ST_Rect_t* frameSize, MI_SYS_BufInfo_t* stBufInfo)
{
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_S32 ret = MI_SUCCESS;

    memset(&stBufConf, 0, sizeof(MI_SYS_BufConf_t));
    memset(&stSrcChnPort, 0, sizeof(stSrcChnPort));

    if(bufHandle == NULL || stBufInfo == NULL)
    {
        printf("Err,Null point \n");
        return -1;
    }

    stBufConf.u64TargetPts = 0;//stTv.tv_sec*1000000 + stTv.tv_usec;
    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.stFrameCfg.u16Width = ALIGN_UP(frameSize->u16PicW, 2);
    stBufConf.stFrameCfg.u16Height = ALIGN_UP(frameSize->u16PicH, 2);
    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.u32Flags = MI_SYS_MAP_VA;

    stSrcChnPort.eModId = E_MI_MODULE_ID_DISP;
    stSrcChnPort.u32ChnId = 0;
    stSrcChnPort.u32DevId = 0;
    stSrcChnPort.u32PortId = 0;

    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

    ret = MI_SYS_ChnInputPortGetBuf(&stSrcChnPort, &stBufConf, stBufInfo, bufHandle, 0);
    if(MI_SUCCESS != ret)
    {
        printf("MI_SYS_ChnInputPortGetBuf,w:%d,h:%d fail,%02x \n",stBufConf.stFrameCfg.u16Height, stBufConf.stFrameCfg.u16Width, ret);
        return -1;
    }
    return 0;
}


int write_JPEG_file (char * filename, unsigned char* yuvData, int quality,int image_width,int image_height)
{
     struct jpeg_compress_struct cinfo;
     struct jpeg_error_mgr jerr;
     FILE * outfile;    // target file 
     JSAMPROW row_pointer[1];  // pointer to JSAMPLE row[s] 
     int row_stride;    // physical row width in image buffer
     JSAMPIMAGE  buffer;   
     unsigned char *pSrc,*pDst;
     int band,i,buf_width[3],buf_height[3];
     cinfo.err = jpeg_std_error(&jerr);

     jpeg_create_compress(&cinfo);

     if ((outfile = fopen(filename, "wb")) == NULL) {

         fprintf(stderr, "can't open %s\n", filename);

         exit(1);

     }

     jpeg_stdio_dest(&cinfo, outfile);
     cinfo.image_width = image_width;  // image width and height, in pixels
     cinfo.image_height = image_height;
     cinfo.input_components = 3;    // # of color components per pixel
     cinfo.in_color_space = JCS_RGB;  //colorspace of input image
     jpeg_set_defaults(&cinfo);
     jpeg_set_quality(&cinfo, quality, TRUE );

     cinfo.raw_data_in = TRUE;
     cinfo.jpeg_color_space = JCS_YCbCr;
     cinfo.comp_info[0].h_samp_factor = 2;
     cinfo.comp_info[0].v_samp_factor = 2;

     jpeg_start_compress(&cinfo, TRUE);
     buffer = (JSAMPIMAGE) (*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo,
                                 JPOOL_IMAGE, 3 * sizeof(JSAMPARRAY)); 

     for(band=0; band <3; band++)
     {
         buf_width[band] = cinfo.comp_info[band].width_in_blocks * DCTSIZE;
         buf_height[band] = cinfo.comp_info[band].v_samp_factor * DCTSIZE;
         buffer[band] = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo,
                                 JPOOL_IMAGE, buf_width[band], buf_height[band]);
     } 

     unsigned char *rawData[3];
     rawData[0]=yuvData;
     rawData[1]=yuvData+image_width*image_height;
     rawData[2]=yuvData+image_width*image_height*5/4;

     int src_width[3],src_height[3];
     for(int i=0;i<3;i++)
     {
         src_width[i]=(i==0)?image_width:image_width/2;
         src_height[i]=(i==0)?image_height:image_height/2;
     }

     //max_line一般为16，外循环每次处理16行数据。
     int max_line = cinfo.max_v_samp_factor*DCTSIZE; 
     for(int counter=0; cinfo.next_scanline < cinfo.image_height; counter++)
     {   
         //buffer image copy.
         for(band=0; band <3; band++)  //每个分量分别处理
         {
              int mem_size = src_width[band];//buf_width[band];
              pDst = (unsigned char *) buffer[band][0];
              pSrc = (unsigned char *) rawData[band] + counter*buf_height[band] * src_width[band];//buf_width[band];  //yuv.data[band]分别表示YUV起始地址
              for(i=0; i <buf_height[band]; i++)  //处理每行数据
              {
                memcpy(pDst, pSrc, mem_size);
                pSrc += src_width[band];//buf_width[band];
                pDst += buf_width[band];
              }
         }
         jpeg_write_raw_data(&cinfo, buffer, max_line);
     }

     jpeg_finish_compress(&cinfo);
     fclose(outfile);
     jpeg_destroy_compress(&cinfo);
     return 0;
}

void myScale(uint8_t *srcYuvData, uint8_t *dstYuvData, int width, int height, int dstWidth, int dstHeight) {
    I420Scale(
            srcYuvData,
            width,
            srcYuvData+width*height,
            (width+1)/2,
            srcYuvData+width*height+((width+1)/2)*((height+1)/2),
            (width+1)/2,
            width,
            height,
            dstYuvData,
            dstWidth,
            dstYuvData+dstWidth*dstWidth,
            (dstWidth+1)/2,
            dstYuvData+dstWidth*dstHeight+((dstWidth+1)/2)*((dstHeight+1)/2),
            (dstWidth+1)/2,
            dstWidth,
            dstHeight,
            kFilterLinear
            );
}
static pj_status_t put_frame(void *data)
{
    struct sigmastar_stream *stream = (struct sigmastar_stream *)data;
    const pjmedia_frame *frame = stream->frame;
    // PJ_LOG(4, (THIS_FILE, "put_frame:%d, %d",stream->param.fmt.id, PJMEDIA_FORMAT_I420));
    // PJ_LOG(4, (THIS_FILE, "put_frame,len:%d, bit_info:%d ,dstrect: %d, %d, %d, %d,rect: %d, %d, %d", frame->size, frame->bit_info, stream->dstrect.s32X, stream->dstrect.s32Y, 
    //     stream->dstrect.u16PicH, stream->dstrect.u16PicW, stream->rect.u16PicH, stream->rect.u16PicW, __LINE__));
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE bufHandle;
    memset(&bufHandle, 0, sizeof(MI_SYS_BUF_HANDLE));
    memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));

    // if (stream->dstrect.u16PicW * stream->dstrect.u16PicH < stream->rect.u16PicW * stream->rect.u16PicH) {
    //     myScale((uint8_t*)frame->buf, (uint8_t*)myFrame.buf, stream->rect.u16PicW, stream->rect.u16PicH, 
    //         stream->dstrect.u16PicW, stream->dstrect.u16PicH);
    //     if(0 == _mi_ChnInputPortGetBuf(&bufHandle, &stream->dstrect, &stBufInfo))
    //     {
    //         _mi_ChnInputPortPutBuf(bufHandle, &stream->dstrect, &stBufInfo, &myFrame);
    //     }
    // }
    // else{
        if(0 == _mi_ChnInputPortGetBuf(&bufHandle, &stream->rect, &stBufInfo))
        {
            _mi_ChnInputPortPutBuf(bufHandle, &stream->rect, &stBufInfo, frame);
        }
    // }
    return PJ_SUCCESS;
}

/* API: Put frame from stream */
static pj_status_t sigmastar_stream_put_frame(pjmedia_vid_dev_stream *strm,
					const pjmedia_frame *frame)
{
    struct sigmastar_stream *stream = (struct sigmastar_stream*)strm;
    pj_status_t status;

    stream->last_ts.u64 = frame->timestamp.u64;

    /* Video conference just trying to send heart beat for updating timestamp
     * or keep-alive, this port doesn't need any, just ignore.
     */
    if (frame->size==0 || frame->buf==NULL)
	return PJ_SUCCESS;

    if (frame->size < stream->vafp.framebytes)
	return PJ_ETOOSMALL;

    
    if (!stream->is_running)
    return PJ_EINVALIDOP;
	
    stream->frame = frame;
    job_queue_post_job(stream->sf->jq, put_frame, strm, 0, &status);
    
    return status;
}

/* API: create stream */
static pj_status_t sigmastar_factory_create_stream(
					pjmedia_vid_dev_factory *f,
					pjmedia_vid_dev_param *param,
					const pjmedia_vid_dev_cb *cb,
					void *user_data,
					pjmedia_vid_dev_stream **p_vid_strm)
{
    struct sigmastar_factory *sf = (struct sigmastar_factory*)f;
    pj_pool_t *pool;
    struct sigmastar_stream *strm;
    pj_status_t status;

    PJ_ASSERT_RETURN(param->dir == PJMEDIA_DIR_RENDER, PJ_EINVAL);

    /* Create and Initialize stream descriptor */
    pool = pj_pool_create(sf->pf, "sigmstar-dev", 1000, 1000, NULL);
    PJ_ASSERT_RETURN(pool != NULL, PJ_ENOMEM);

    strm = PJ_POOL_ZALLOC_T(pool, struct sigmastar_stream);
    pj_memcpy(&strm->param, param, sizeof(*param));
    strm->pool = pool;
    strm->sf = sf;
    pj_memcpy(&strm->vid_cb, cb, sizeof(*cb));
    pj_list_init(&strm->list_entry);
    strm->list_entry.stream = strm;
    strm->user_data = user_data;

    /* Create render stream here */
    job_queue_post_job(sf->jq, sigmastar_create, strm, 0, &status);
    if (status != PJ_SUCCESS) {
        goto on_error;
    }
    pj_mutex_lock(strm->sf->mutex);
    if (pj_list_empty(&strm->sf->streams))
        pj_sem_post(strm->sf->sem);
    pj_list_insert_after(&strm->sf->streams, &strm->list_entry);
    pj_mutex_unlock(strm->sf->mutex);

    /* Done */
    strm->base.op = &stream_op;
    *p_vid_strm = &strm->base;

    return PJ_SUCCESS;

on_error:
    sigmastar_stream_destroy(&strm->base);
    return status;
}

/* API: Get stream info. */
static pj_status_t sigmastar_stream_get_param(pjmedia_vid_dev_stream *s,
					pjmedia_vid_dev_param *pi)
{
    struct sigmastar_stream *strm = (struct sigmastar_stream*)s;

    PJ_ASSERT_RETURN(strm && pi, PJ_EINVAL);

    pj_memcpy(pi, &strm->param, sizeof(*pi));

    if (sigmastar_stream_get_cap(s, PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW,
			   &pi->window) == PJ_SUCCESS)
    {
	    pi->flags |= PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW;
    }
    if (sigmastar_stream_get_cap(s, PJMEDIA_VID_DEV_CAP_OUTPUT_POSITION,
			   &pi->window_pos) == PJ_SUCCESS)
    {
	pi->flags |= PJMEDIA_VID_DEV_CAP_OUTPUT_POSITION;
    }
    if (sigmastar_stream_get_cap(s, PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE,
			   &pi->disp_size) == PJ_SUCCESS)
    {
	pi->flags |= PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE;
    }
    if (sigmastar_stream_get_cap(s, PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE,
			   &pi->window_hide) == PJ_SUCCESS)
    {
	pi->flags |= PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE;
    }
    if (sigmastar_stream_get_cap(s, PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW_FLAGS,
			   &pi->window_flags) == PJ_SUCCESS)
    {
	pi->flags |= PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW_FLAGS;
    }
    if (sigmastar_stream_get_cap(s, PJMEDIA_VID_DEV_CAP_OUTPUT_FULLSCREEN,
			   &pi->window_fullscreen) == PJ_SUCCESS)
    {
	pi->flags |= PJMEDIA_VID_DEV_CAP_OUTPUT_FULLSCREEN;
    }

    return PJ_SUCCESS;
}

struct strm_cap {
    struct sigmastar_stream   *strm;
    pjmedia_vid_dev_cap  cap;
    union {
        void            *pval;
        const void      *cpval;
    } pval;
};

static pj_status_t get_cap(void *data)
{
    struct strm_cap *scap = (struct strm_cap *)data;
    struct sigmastar_stream *strm = scap->strm;
    pjmedia_vid_dev_cap cap = scap->cap;
    void *pval = scap->pval.pval;
    
    if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW)
    {
        pjmedia_vid_dev_hwnd *wnd = (pjmedia_vid_dev_hwnd *)pval;
        wnd->type = PJMEDIA_VID_DEV_HWND_TYPE_SIGMASTAR;
        wnd->info.sigmastar.window = 1;
        PJ_LOG(4, (THIS_FILE, "PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW error %d", __LINE__));
	    return PJ_SUCCESS;
    } else if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_POSITION) {
        MI_DISP_InputPortAttr_t stInputPortAttr;
        memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));
        MI_S32 ret = MI_DISP_GetInputPortAttr(0, 0, &stInputPortAttr);
        ((pjmedia_coord *)pval)->x = stInputPortAttr.stDispWin.u16X;
        ((pjmedia_coord *)pval)->y = stInputPortAttr.stDispWin.u16Y;
	    return PJ_SUCCESS;
    } else if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE) {
        MI_DISP_InputPortAttr_t stInputPortAttr;
        memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));
        MI_S32 ret = MI_DISP_GetInputPortAttr(0, 0, &stInputPortAttr);
        ((pjmedia_rect_size *)pval)->w = stInputPortAttr.stDispWin.u16Width;
        ((pjmedia_rect_size *)pval)->h = stInputPortAttr.stDispWin.u16Height;
	    return PJ_SUCCESS;
    } else if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE) {
        PJ_LOG(4, (THIS_FILE, "PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE error %d", __LINE__));
	    return PJ_SUCCESS;
    } else if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW_FLAGS) {
        PJ_LOG(4, (THIS_FILE, "PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW_FLAGS error %d", __LINE__));
	    return PJ_SUCCESS;
    } else if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_FULLSCREEN) {
        PJ_LOG(4, (THIS_FILE, "PJMEDIA_VID_DEV_CAP_OUTPUT_FULLSCREEN error %d", __LINE__));
	    return PJ_SUCCESS;
    }

    return PJMEDIA_EVID_INVCAP;
}

/* API: get capability */
static pj_status_t sigmastar_stream_get_cap(pjmedia_vid_dev_stream *s,
				      pjmedia_vid_dev_cap cap,
				      void *pval)
{
    struct sigmastar_stream *strm = (struct sigmastar_stream*)s;
    struct strm_cap scap;
    pj_status_t status;

    PJ_ASSERT_RETURN(s && pval, PJ_EINVAL);

    scap.strm = strm;
    scap.cap = cap;
    scap.pval.pval = pval;

    job_queue_post_job(strm->sf->jq, get_cap, &scap, 0, &status);

    return status;
}

static pj_status_t set_cap(void *data)
{
    struct strm_cap *scap = (struct strm_cap *)data;
    struct sigmastar_stream *strm = scap->strm;
    pjmedia_vid_dev_cap cap = scap->cap;
    const void *pval = scap->pval.cpval;
    PJ_LOG(4, (THIS_FILE, "set_cap:%d, %d", cap, __LINE__));
    if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_POSITION) {
        /**
         * Setting window's position when the window is hidden also sets
         * the window's flag to shown (while the window is, actually,
         * still hidden). This causes problems later when setting/querying
         * the window's visibility.
         * See ticket #1429 (http://trac.pjsip.org/repos/ticket/1429)
         */
        return reposition_disp(strm, ((pjmedia_coord *)pval)->x, ((pjmedia_coord *)pval)->y);
    } 
    else if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE) {
        PJ_LOG(4, (THIS_FILE, "PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE error %d", __LINE__));
	    return PJ_SUCCESS;
    } 
    else if (cap == PJMEDIA_VID_DEV_CAP_FORMAT) {
        pj_status_t status;

        status = change_format(strm, (pjmedia_format *)pval);
        if (status != PJ_SUCCESS) {
            pj_status_t status_;
            PJ_LOG(4, (THIS_FILE, "PJMEDIA_VID_DEV_CAP_FORMAT 11 error %d", __LINE__));
            /**
             * Failed to change the output format. Try to revert
             * to its original format.
             */
                status_ = change_format(strm, &strm->param.fmt);
            if (status_ != PJ_SUCCESS) {
                /**
                 * This means that we failed to revert to our
                 * original state!
                 */
                PJ_LOG(4, (THIS_FILE, "PJMEDIA_VID_DEV_CAP_FORMAT 222 error %d", __LINE__));
                status = PJMEDIA_EVID_ERR;
            }
        }
	    return status;
    } 
    else if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE) {
	    pjmedia_rect_size *new_size = (pjmedia_rect_size *)pval;
        return resize_disp(strm, new_size);
    } 
    else if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW) {
        pjmedia_vid_dev_hwnd *hwnd = (pjmedia_vid_dev_hwnd*)pval;
        pj_status_t status = PJ_SUCCESS;
        sigmastar_fmt_info *sigmastar_info = get_sigmastar_format_info(strm->param.fmt.id);
        /* Re-init sigmastar */
        status = sigmastar_destroy_all(strm);
        if (status != PJ_SUCCESS)
            return status;	

        status = sigmastar_create_window(strm, PJ_TRUE, sigmastar_info->sigmastar_format, hwnd);
            // PJ_PERROR(4, (THIS_FILE, status,
            //     "Re-initializing sigmastar with native window %d",
            //     hwnd->info.window));
        return status;	
    } 
    else if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_FULLSCREEN) {
	    PJ_LOG(4, (THIS_FILE, "PJMEDIA_VID_DEV_CAP_OUTPUT_FULLSCREEN not support,%d", __LINE__));
        return MI_SUCCESS;
    }

    return PJMEDIA_EVID_INVCAP;
}

/* API: set capability */
static pj_status_t sigmastar_stream_set_cap(pjmedia_vid_dev_stream *s,
				      pjmedia_vid_dev_cap cap,
				      const void *pval)
{
    struct sigmastar_stream *strm = (struct sigmastar_stream*)s;
    struct strm_cap scap;
    pj_status_t status;

    PJ_ASSERT_RETURN(s && pval, PJ_EINVAL);

    scap.strm = strm;
    scap.cap = cap;
    scap.pval.cpval = pval;

    job_queue_post_job(strm->sf->jq, set_cap, &scap, 0, &status);

    return status;
}

/* API: Start stream. */
static pj_status_t sigmastar_stream_start(pjmedia_vid_dev_stream *strm)
{
    struct sigmastar_stream *stream = (struct sigmastar_stream*)strm;

    PJ_LOG(4, (THIS_FILE, "Starting sigmastar video stream"));

    stream->is_running = PJ_TRUE;

    return PJ_SUCCESS;
}


/* API: Stop stream. */
static pj_status_t sigmastar_stream_stop(pjmedia_vid_dev_stream *strm)
{
    struct sigmastar_stream *stream = (struct sigmastar_stream*)strm;

    PJ_LOG(4, (THIS_FILE, "Stopping sigmastar video stream, %d", __LINE__));

    stream->is_running = PJ_FALSE;

    return PJ_SUCCESS;
}


/* API: Destroy stream. */
static pj_status_t sigmastar_stream_destroy(pjmedia_vid_dev_stream *strm)
{
    struct sigmastar_stream *stream = (struct sigmastar_stream*)strm;
    pj_status_t status;

    PJ_ASSERT_RETURN(stream != NULL, PJ_EINVAL);

    sigmastar_stream_stop(strm);

    job_queue_post_job(stream->sf->jq, sigmastar_destroy_all, strm, 0, &status);
    if (status != PJ_SUCCESS)
        return status;

    pj_mutex_lock(stream->sf->mutex);
    if (!pj_list_empty(&stream->list_entry))
	pj_list_erase(&stream->list_entry);
    pj_mutex_unlock(stream->sf->mutex);

    pj_pool_release(stream->pool);

    return PJ_SUCCESS;
}

/****************************************************************************
 * Job queue implementation
 */
#if PJ_DARWINOS==0
static int job_thread(void * data)
{
    job_queue *jq = (job_queue *)data;

    while (1) {
        job *jb;

	/* Wait until there is a job. */
        pj_sem_wait(jq->sem);

        /* Make sure there is no pending jobs before we quit. */
        if (jq->is_quitting && jq->head == jq->tail && !jq->is_full)
            break;

        jb = jq->jobs[jq->head];
        jb->retval = (*jb->func)(jb->data);
        /* If job queue is full and we already finish all the pending
         * jobs, increase the size.
         */
        if (jq->is_full && ((jq->head + 1) % jq->size == jq->tail)) {
            unsigned i, head;
            pj_status_t status;

            if (jq->old_sem) {
                for (i = 0; i < jq->size / JOB_QUEUE_INC_FACTOR; i++) {
                    pj_sem_destroy(jq->old_sem[i]);
                }
            }
            jq->old_sem = jq->job_sem;

            /* Double the job queue size. */
            jq->size *= JOB_QUEUE_INC_FACTOR;
            pj_sem_destroy(jq->sem);
            status = pj_sem_create(jq->pool, "thread_sem", 0, jq->size + 1,
                                   &jq->sem);
            if (status != PJ_SUCCESS) {
                PJ_PERROR(3, (THIS_FILE, status,
			      "Failed growing sigmastar job queue size."));
                return 0;
            }
            jq->jobs = (job **)pj_pool_calloc(jq->pool, jq->size,
                                              sizeof(job *));
            jq->job_sem = (pj_sem_t **) pj_pool_calloc(jq->pool, jq->size,
                                                       sizeof(pj_sem_t *));
            for (i = 0; i < jq->size; i++) {
                status = pj_sem_create(jq->pool, "job_sem", 0, 1,
                                       &jq->job_sem[i]);
                if (status != PJ_SUCCESS) {
                    PJ_PERROR(3, (THIS_FILE, status,
				  "Failed growing sigmastar job queue size."));
                    return 0;
                }
            }
            jq->is_full = PJ_FALSE;
            head = jq->head;
            jq->head = jq->tail = 0;
            pj_sem_post(jq->old_sem[head]);
        } else {
            pj_sem_post(jq->job_sem[jq->head]);
            jq->head = (jq->head + 1) % jq->size;
        }
    }

    return 0;
}
#endif

static pj_status_t job_queue_create(pj_pool_t *pool, job_queue **pjq)
{
    unsigned i;
    pj_status_t status;

    job_queue *jq = PJ_POOL_ZALLOC_T(pool, job_queue);
    jq->pool = pool;
    jq->size = INITIAL_MAX_JOBS;
    status = pj_sem_create(pool, "thread_sem", 0, jq->size + 1, &jq->sem);
    if (status != PJ_SUCCESS)
        goto on_error;
    jq->jobs = (job **)pj_pool_calloc(pool, jq->size, sizeof(job *));
    jq->job_sem = (pj_sem_t **) pj_pool_calloc(pool, jq->size,
                                               sizeof(pj_sem_t *));
    for (i = 0; i < jq->size; i++) {
        status = pj_sem_create(pool, "job_sem", 0, 1, &jq->job_sem[i]);
        if (status != PJ_SUCCESS)
            goto on_error;
    }

    status = pj_mutex_create_recursive(pool, "job_mutex", &jq->mutex);
    if (status != PJ_SUCCESS)
        goto on_error;

#if defined(PJ_DARWINOS) && PJ_DARWINOS!=0
    PJ_UNUSED_ARG(status);
#else
    status = pj_thread_create(pool, "job_th", job_thread, jq, 0, 0,
                              &jq->thread);
    if (status != PJ_SUCCESS)
        goto on_error;
#endif /* PJ_DARWINOS */

    *pjq = jq;
    return PJ_SUCCESS;

on_error:
    job_queue_destroy(jq);
    return status;
}

static pj_status_t job_queue_post_job(job_queue *jq, job_func_ptr func,
				      void *data, unsigned flags,
				      pj_status_t *retval)
{
    job jb;
    int tail;

    if (jq->is_quitting)
        return PJ_EBUSY;

    jb.func = func;
    jb.data = data;
    jb.flags = flags;

    pj_mutex_lock(jq->mutex);
    jq->jobs[jq->tail] = &jb;
    tail = jq->tail;
    jq->tail = (jq->tail + 1) % jq->size;
    if (jq->tail == jq->head) {
	jq->is_full = PJ_TRUE;
        PJ_LOG(4, (THIS_FILE, "sigmastar job queue is full, increasing "
                              "the queue size."));
        pj_sem_post(jq->sem);
        /* Wait until our posted job is completed. */
        pj_sem_wait(jq->job_sem[tail]);
        pj_mutex_unlock(jq->mutex);
    } else {
        pj_mutex_unlock(jq->mutex);
        pj_sem_post(jq->sem);
        /* Wait until our posted job is completed. */
        pj_sem_wait(jq->job_sem[tail]);
    }

    *retval = jb.retval;

    return PJ_SUCCESS;
}

static pj_status_t job_queue_destroy(job_queue *jq)
{
    unsigned i;

    jq->is_quitting = PJ_TRUE;

    if (jq->thread) {
        pj_sem_post(jq->sem);
        pj_thread_join(jq->thread);
        pj_thread_destroy(jq->thread);
    }

    if (jq->sem) {
        pj_sem_destroy(jq->sem);
        jq->sem = NULL;
    }
    for (i = 0; i < jq->size; i++) {
        if (jq->job_sem[i]) {
            pj_sem_destroy(jq->job_sem[i]);
            jq->job_sem[i] = NULL;
        }
    }
    if (jq->old_sem) {
        for (i = 0; i < jq->size / JOB_QUEUE_INC_FACTOR; i++) {
            if (jq->old_sem[i]) {
                pj_sem_destroy(jq->old_sem[i]);
                jq->old_sem[i] = NULL;
            }
        }
    }
    if (jq->mutex) {
        pj_mutex_destroy(jq->mutex);
        jq->mutex = NULL;
    }

    return PJ_SUCCESS;
}

#endif	/* PJMEDIA_VIDEO_DEV_HAS_SIGMASTAR */
