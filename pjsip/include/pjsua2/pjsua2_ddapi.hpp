#ifndef __PJSUA2_DDAPI_HPP__
#define __PJSUA2_DDAPI_HPP__
#include <pjsua2/account.hpp>
#include <pjsua2/call.hpp>
#include <pjsua2/media.hpp>
#include <pjsua2/endpoint.hpp>
#include <memory>

class DdAccount;
class DdCall;
using namespace pj;
typedef struct pjsip_account_info {
	std::string str_domain; //domain hj.doorudu.com
	std::string str_acct;
	std::string str_pwd;
	std::string str_log_file;
	std::string str_outcall_file;
	std::string str_incall_file;
	int n_port;  //本机绑定端口
	int n_port_range; //本机绑定端口范围
	int n_model; //udp/tls/tcp  0/1/2
	bool b_g729;
	bool b_722;
	bool b_logAppend;
	bool b_pcma;
	bool b_pcmu;
	bool b_gsm;
	bool b_h264;
	bool b_vp8;
	bool b_vp9;
	bool b_ice;
	int n_log_level;
	int de_fps;
	int de_width;
	int de_height;
	int de_avg_bps;
	int en_fps;
	int en_width;
	int en_height;
	int en_avg_bps;
	int n_retry_reg_intervel; //重新注册时间间隔，单位秒
	bool b_ignorefmtp; //释放忽略sdp协商
	bool bP2p;
	pjsip_account_info() {
		str_domain = "";
		str_acct = "";
		str_pwd = "";
		str_log_file = "pjsua2_ddapi.log";
		str_outcall_file = "";
		str_incall_file = "";
		n_port = 5060;
		n_port_range = 0;
		n_model = 0;
		b_g729 = true;
		b_logAppend = false;
		b_722 = true;
		b_pcma = true;
		b_pcmu = true;
		b_gsm = false;
		b_h264 = true;
		b_vp8 = false;
		b_vp9 = false; 
		b_ice = false;
		n_log_level = 4;
		de_fps = 15;
		de_width = 640;
		de_height = 480;
		de_avg_bps = 512000;

		en_fps = 15;
		en_width = 640;
		en_height = 480;
		en_avg_bps = 512000;
		n_retry_reg_intervel = 10; 
		b_ignorefmtp = true;
		bP2p = false;
	}
}pjsip_account_info;

typedef struct {
	int n_acct_id;
	int n_call_id;
	std::string str_call_id;
	std::string str_acc_name;
}pjsip_dialog_id;

typedef enum {
	E_INCOMMING_CALL,
	E_CALL_MEDIA_AUDIO,
	E_CALL_CONFIRMED,
	E_CALL_MEDIA_VIDEO,
	E_CALL_VIDEO_FOUND_KEYFRAME,
	E_CALL_HANGUP,
	E_CALL_MAX
}PJSIP_DDAPI_CALL_STATE;

typedef enum {
	E_SIPDD_SUCCESS = 0,
	E_PARA_ERROR,
	E_HAS_NOT_OPERATE,
	E_HAS_OPERATED,
	E_CREATE_SIP_ERROR,
	E_INIT_SIP_ERROR,
	E_CREATE_UDP_ERROR,
	E_CREATE_TCP_ERROR,
	E_CREATE_TLS_ERROR,
	E_ADD_ACC_ERROR,
	E_START_ACC_ERROR,
	E_NO_ACCT_ERROR,
	E_GET_AUDIO_CODEC_ERROR,
	E_SET_AUDIO_CODEC_ERROR,
	E_GET_VIDEO_CODEC_ERROR,
	E_SET_VIDEO_CODEC_ERROR,
	E_ENABLE_VIDEO_ERROR,
	E_CALL_OUT_ERROR,
	E_ANSWER_ERROR,
	E_HANGUP_ERROR,
	E_BUSY_ERROR,
	E_PREVIEW_ERROR,
	E_MUTE_ERROR,
	E_SDL_ERROR,
	E_GET_CALLINFO_ERROR,
	E_NO_SOUND_DEVICE,
	E_OTHER_ERROR
}PJSIP_DDAPI_STATUS_CODE;

#define PJSIP_MODEL_UDP 0
#define PJSIP_MODEL_TLS 1
#define PJSIP_MODEL_TCP 2

class PjsuaVideoWindow {
public:
	PjsuaVideoWindow(int n_wnd_id, void *Handle);

	void setSize(int n_high, int n_wide);

	void setPose(int n_x, int n_y);

	void show(bool b_show);

private:
	void *m_handle;
	int m_nWndId;
};


class PjsuaDDApiCb {
public:
	//
	virtual void onCallState(PJSIP_DDAPI_CALL_STATE n_type, const std::string& str_name, const std::string& str_ip, int n_port,
		int n_call_id, const std::string &str_xtran) = 0;

	virtual void onVideocomingCall(const std::string& str_name, const std::string& str_ip, int n_port,
		int n_call_id, PjsuaVideoWindow videoWindow)  = 0;

	virtual void onHanguped(int n_call_id, const std::string& str_acct, int hangup_code) = 0;

	virtual void onRegisterState(int n_status, std::string strReason) = 0;

	virtual void onOtherState(pjsip_inv_state state) = 0;

	virtual ~PjsuaDDApiCb() {};

	std::string m_strXtran;

};

class PjsuaDDApiInterface {
public:
	static PjsuaDDApiInterface &instance() PJSUA2_THROW(Error);
	//接口类，需要传入配置信息和回调函数
	PjsuaDDApiInterface(const pjsip_account_info &acc_info, PjsuaDDApiCb& api_cb);
	//注册
	PJSIP_DDAPI_STATUS_CODE registerAcct();

	PJSIP_DDAPI_STATUS_CODE registerAcct(std::string strDomain, std::string strAcct, std::string strPwd);

	PJSIP_DDAPI_STATUS_CODE unregisterAcct();

	//账号上线
	PJSIP_DDAPI_STATUS_CODE Online();

	//账号下线
	PJSIP_DDAPI_STATUS_CODE Offline();

	//对当前会话示忙
	PJSIP_DDAPI_STATUS_CODE busyCurrentCall(int n_call_id);

	// 向外打电话
	int outCall(const std::string & str_acct, const std::string &str_ip, int n_port, const std::string &str_xtran, bool bVideo);

	//挂断当前会话
	// PJSIP_DDAPI_STATUS_CODE hangupCurrentCall(int n_call_id);
	PJSIP_DDAPI_STATUS_CODE hangupCurrentCall(int n_call_id, pjsip_status_code stCode = PJSIP_SC_NULL);

	//接听当前会话
	PJSIP_DDAPI_STATUS_CODE answerCurrentCall(int n_call_id, pjsip_status_code stCode = PJSIP_SC_OK);

	//使能或者去使能当前的视频采集
	PJSIP_DDAPI_STATUS_CODE enableVideo(int n_call_id, bool bVideo);
	//静音
	PJSIP_DDAPI_STATUS_CODE muteCurrentCall(int n_call_id, bool b_mute);
	//拍照
	PJSIP_DDAPI_STATUS_CODE capturePicture(int n_call_id, const std::string& strFileName);
	//获取图片质量
	std::string getStreamQuality(int n_call_id);
	////获取string 类型的id
	//std::string getCallIdStr(int n_call_id);

	virtual~PjsuaDDApiInterface();

	//获取输出音量
	unsigned getOutputVolume();
	//设置输入音量
	PJSIP_DDAPI_STATUS_CODE setOutputVolume(unsigned volume);
	//获取输入音量
	unsigned getInputVolume();
	//设置输入音量
	PJSIP_DDAPI_STATUS_CODE setInputVolume(unsigned volume);

	bool isCalling();

	PJSIP_DDAPI_STATUS_CODE incommingCallStartRinging(int n_call_id);

private:

	void setCalling(std::shared_ptr<Call> pCall) { m_pCurrentCall = pCall; };

	std::shared_ptr<Call> getCalling() { return m_pCurrentCall; };

	PJSIP_DDAPI_STATUS_CODE hangupCurrentCall_internal(int n_call_id, pjsip_status_code stCode = PJSIP_SC_NULL);

	std::string strGetOutCallFile(){
		return m_myAcctInfo.str_outcall_file;
	}

	std::string strGetInCallFile(){
		return m_myAcctInfo.str_incall_file;
	}

	friend class DdCall;
	friend class DdAccount;
private:
	pjsip_account_info m_myAcctInfo;
	Account *m_pAcct;
	std::shared_ptr<Call> m_pCurrentCall;
	PjsuaDDApiCb *m_pjsua2Cb;
	EpConfig m_epCfg;
	Endpoint m_ep;
	AccountConfig *m_paCfg;
	static PjsuaDDApiInterface* instance_;
	unsigned m_nInput;
	unsigned m_nOutput;
	bool bHasSound;
};

#endif //__PJSUA2_DDAPI_HPP__