
#include <pjsua2/pjsua2_ddapi.hpp>
#include <algorithm>  
#include <future>
#include <iostream>
#include <memory>

#define THIS_FILE "pjsua2_ddapi.cpp"
#define DD_UNUSED(x) (void)x;

PjsuaDDApiInterface *PjsuaDDApiInterface::instance_;

void pjcall_get_remote_acc_info(const std::string& remote_info, const std::string &remote_contact, std::string& str_acct, int &n_port, std::string& str_ip) {
	n_port = 0;
	if (!remote_info.empty()) {

		size_t begin_pos = remote_info.find_first_of("sip:");
		size_t end_pos = remote_info.find_first_of("@");

		if (begin_pos != std::string::npos && end_pos != std::string::npos && begin_pos < end_pos) {
			str_acct = remote_info.substr(begin_pos + 4, end_pos - begin_pos - 4);
		}
		else {
			str_acct = remote_info;
		}
	}
	if (!remote_contact.empty()) {
		size_t begin_pos = remote_contact.find_first_of("@");
		if (begin_pos != std::string::npos) {
			std::string str_substr = remote_contact.substr(begin_pos + 1);

			begin_pos = str_substr.find_first_of(":");
			if (begin_pos != std::string::npos) {
				str_ip = str_substr.substr(0, begin_pos);

				size_t end_pos = str_substr.find_first_of('>');
				if (end_pos != std::string::npos) {
					std::string str_tmp = str_substr.substr(begin_pos + 1,end_pos - begin_pos - 1);
					n_port = atoi(str_tmp.c_str());
					return;
				}
				else {
					std::string str_tmp = str_substr.substr(begin_pos + 1);
					n_port = atoi(str_tmp.c_str());
					return;
				}
			}
		}
	}
}

std::string get_x_tran(const std::string& strdata) {

	size_t npos_begin = strdata.find("X-Tran:");
	if (npos_begin == std::string::npos) {
		return "";
	}
	std::string strSub = strdata.substr(npos_begin + strlen("X-Tran:"));
	size_t npos_end = strSub.find("\r\n");
	if (npos_end == std::string::npos) {
		npos_end = strSub.find("\n");
	}
	if (npos_end == std::string::npos) {
		;
	}
	else{
		strSub = strSub.substr(0,  npos_end);
	}
	npos_begin = strSub.find_first_not_of(" ");
	return strSub.substr(npos_begin);
}

PjsuaVideoWindow::PjsuaVideoWindow(int n_wnd_id, void* handle):
	m_nWndId(n_wnd_id), m_handle(handle)
{}

void PjsuaVideoWindow::setSize(int n_high, int n_wide) {
	if (m_nWndId < 0) {
		return;
	}
	VideoWindow vWnd = VideoWindow(m_nWndId);
	MediaSize size;
	size.h = n_high;
	size.w = n_wide;
	try {
		vWnd.setSize(size);
	}
	catch (Error &err) {
		PJ_LOG(3, (THIS_FILE, "setSize %s:%d", err.info().c_str(), __LINE__));
	}
}
void PjsuaVideoWindow::show(bool b_show) {
	if (m_nWndId < 0) {
		return;
	}
	VideoWindow vWnd = VideoWindow(m_nWndId);
	try {
		vWnd.Show(b_show);
	}
	catch (Error &err) { 
		PJ_LOG(3, (THIS_FILE, "show %s:%d", err.info().c_str(), __LINE__)); 
	}
}

void PjsuaVideoWindow::setPose(int n_x, int n_y) {
	if (m_nWndId < 0) {
		return;
	}
	VideoWindow vWnd = VideoWindow(m_nWndId);
	MediaCoordinate pos = { n_x, n_y };
	try {
		vWnd.setPos(pos);
	}
	catch (Error &err) {
		PJ_LOG(3, (THIS_FILE, "setPos %s:%d", err.info().c_str(), __LINE__));
	}
}
#define RING_FREQ1	    800
#define RING_FREQ2	    640
#define RING_ON		    200
#define RING_OFF	    100
#define RING_CNT	    3
#define RING_INTERVAL	    3000

class DdCall : public Call
{
public:
	void initTonegen(const std::string &strSoundfile) {
		if (p_myPlayer == NULL && !strSoundfile.empty()){
			p_myPlayer = new AudioMediaPlayer();
			p_myPlayer->createPlayer(strSoundfile, 0); 
		}
	}

	void ringStop()
	{
		if (p_myPlayer != NULL && !bHasStop){
			AudDevManager& mgr = Endpoint::instance().audDevManager();
			PJ_LOG(3, (THIS_FILE, "ringStop:%d, %d", mgr.getPlaybackDev(), __LINE__));
			p_myPlayer->stopTransmit(mgr.getPlaybackDevMedia());
		}
		bHasStop = true;
	}

	void ringStart()
	{
		if (p_myPlayer != NULL) {
			AudDevManager& mgr = Endpoint::instance().audDevManager();

		PJ_LOG(3, (THIS_FILE, "ringStart:%d, %d", mgr.getPlaybackDev(), __LINE__));
			p_myPlayer->startTransmit(mgr.getPlaybackDevMedia());
		}
	}


	DdCall(Account &acc, int call_id = PJSUA_INVALID_ID) : Call(acc, call_id), nCapDev(PJMEDIA_VID_DEFAULT_CAPTURE_DEV), 
		n_wind_id(-1), p_myPlayer(NULL), bHasStop(false)
	{
		// initTonegen();
	}
	~DdCall()
	{
		if (p_myPlayer != NULL) {
			delete p_myPlayer;
			p_myPlayer = NULL;
		}
	}
	// Notification when call's state has changed.
	virtual void onCallState(OnCallStateParam &prm) {
		DD_UNUSED(prm);
		CallInfo ci = getInfo();
		PJ_LOG(3, (THIS_FILE, "onCallState.id:%d, state:%d, role:%d, remote_info:%s, remote_contact:%s, %d", 
			ci.id, ci.state, ci.role, ci.remoteUri.c_str(), ci.remoteContact.c_str(), __LINE__));
		if (ci.state == PJSIP_INV_STATE_DISCONNECTED)
		{
			ringStop();
			std::string str_acct = "";
			std::string str_ip = "";
			int n_port = 0;
			pjcall_get_remote_acc_info(ci.remoteUri, ci.remoteContact, str_acct, n_port, str_ip);
			PJ_LOG(3, (THIS_FILE, "pjcall_get_remote_acc_info.str_acct:%s, n_port:%d, str_ip:%s, remoteUri:%s, remoteContact:%s, %d, %d",
				str_acct.c_str(), n_port, str_ip.c_str(), ci.remoteUri.c_str(), ci.remoteContact.c_str(), ci.lastStatusCode, __LINE__));
			PjsuaDDApiInterface::instance().m_pjsua2Cb->onHanguped(ci.id, str_acct, ci.lastStatusCode);
		}
		else if (ci.state == PJSIP_INV_STATE_CONFIRMED) {
			std::string str_acct = "";
			std::string str_ip = "";
			int n_port = 0;
			pjcall_get_remote_acc_info(ci.remoteUri, ci.remoteContact, str_acct, n_port, str_ip);
			PJ_LOG(3, (THIS_FILE, "pjcall_get_remote_acc_info.str_acct:%s, n_port:%d, str_ip:%s, remoteUri:%s, remoteContact:%s, %d",
				str_acct.c_str(), n_port, str_ip.c_str(), ci.remoteUri.c_str(), ci.remoteContact.c_str(), __LINE__));
			PjsuaDDApiInterface::instance().m_pjsua2Cb->onCallState(E_CALL_CONFIRMED, str_acct, str_ip, n_port, ci.id, "");

		}
		else{
			PJ_LOG(4, (THIS_FILE, "prm.e.body.txMsg.tdata.wholeMsg:%s, %d", prm.e.body.txMsg.tdata.info.c_str(), prm.e.body.txMsg.tdata.info.size()));
			// std::string xtran = get_x_tran(prm.e.body.txMsg.tdata.wholeMsg);
			PjsuaDDApiInterface::instance().m_pjsua2Cb->onOtherState(ci.state);
		}

	}

	virtual void onCallMediaState(OnCallMediaStateParam &prm) {
		ringStop();
		DD_UNUSED(prm);
		CallInfo ci = getInfo();
		PJ_LOG(3, (THIS_FILE, "onCallMediaState.id:%d, role:%d, remote_info:%s, remote_contact:%s",
			ci.id, ci.role, ci.remoteUri.c_str(), ci.remoteContact.c_str()));
		// Iterate all the call medias
		std::string str_acct = "";
		std::string str_ip = "";
		int n_port = 0;
		pjcall_get_remote_acc_info(ci.remoteUri, ci.remoteContact, str_acct, n_port, str_ip);

		if (bhangingup) {
			PJ_LOG(3, (THIS_FILE, "onCallMediaState hanguped, %d", __LINE__));
		}
		for (unsigned i = 0; i < ci.media.size(); i++) {
			PJ_LOG(3, (THIS_FILE, "ci.media[%d].type:%d, status:%d", i, ci.media[i].type, ci.media[i].status));

			if (ci.media[i].type == PJMEDIA_TYPE_AUDIO && (ci.media[i].status == PJSUA_CALL_MEDIA_ACTIVE || ci.media[i].status == PJSUA_CALL_MEDIA_REMOTE_HOLD)) {
				
				if (!bhangingup) {
					try {
						AudioMedia audioMedia = getAudioMedia(i);
						// Connect the call audio media to sound device
						AudDevManager& mgr = Endpoint::instance().audDevManager();
						audioMedia.startTransmit(mgr.getPlaybackDevMedia());
						
						mgr.setInputVolume(80);

						float rx_level = (float)128 / 128;
						float tx_level = (float)128 / 128;
						
						PJ_LOG(3, (THIS_FILE, "rx_level:%f, tx_level:%f, %d", rx_level, tx_level, __LINE__));

						audioMedia.adjustRxLevel(rx_level);
						audioMedia.adjustTxLevel(tx_level);
						AudioMedia * captureMedia = &(mgr.getCaptureDevMedia());
						captureMedia->adjustRxLevel(rx_level);
						captureMedia->adjustTxLevel(tx_level);
						captureMedia->startTransmit(audioMedia);
					}
					catch (Error &err) { PJ_LOG(3, (THIS_FILE, "onCallMediaState %s:%d", err.info().c_str(), __LINE__)); }
				}
				PJ_LOG(3, (THIS_FILE, "pjcall_get_remote_acc_info.str_acct:%s, n_port:%d, str_ip:%s, remoteUri:%s, remoteContact:%s, %d",
					str_acct.c_str(), n_port, str_ip.c_str(), ci.remoteUri.c_str(), ci.remoteContact.c_str(), __LINE__));
				PjsuaDDApiInterface::instance().m_pjsua2Cb->onCallState(E_CALL_MEDIA_AUDIO, str_acct, str_ip, n_port, ci.id, "");
			}
			else if (ci.media[i].type == PJMEDIA_TYPE_VIDEO && ci.media[i].status == PJSUA_CALL_MEDIA_ACTIVE && ci.media[i].videoIncomingWindowId != INVALID_ID) {
				pjsua_vid_win_info wi;
				pjsua_vid_win_get_info(ci.media[i].videoIncomingWindowId, &wi);
				n_wind_id = ci.media[i].videoIncomingWindowId;
				handle = wi.hwnd.info.win.hwnd;
				PJ_LOG(3, (THIS_FILE, "pjcall_get_remote_acc_info.str_acct:%s, nCapDev:%d, n_port:%d, str_ip:%s, remoteUri:%s, remoteContact:%s, windid:%d, %d",
					str_acct.c_str(), nCapDev, n_port, str_ip.c_str(), ci.remoteUri.c_str(), ci.remoteContact.c_str(), n_wind_id, __LINE__));
				PjsuaDDApiInterface::instance().m_pjsua2Cb->onVideocomingCall(str_acct, str_ip, n_port, ci.id, 
					PjsuaVideoWindow(ci.media[i].videoIncomingWindowId, wi.hwnd.info.win.hwnd));
			}
		}
	}

	virtual void onCallTransferRequest(OnCallTransferRequestParam &prm) { DD_UNUSED(prm); }

	virtual void onCallTransferStatus(OnCallTransferStatusParam &prm) { DD_UNUSED(prm); }

	virtual void onCallReplaceRequest(OnCallReplaceRequestParam &prm) { DD_UNUSED(prm); }

	virtual void onCallMediaEvent(OnCallMediaEventParam &prm) {
		CallInfo ci = getInfo();
		std::string str_acct = "";
		std::string str_ip = "";
		int n_port = 0;
		pjcall_get_remote_acc_info(ci.remoteUri, ci.remoteContact, str_acct, n_port, str_ip);
		PjsuaDDApiInterface::instance().m_pjsua2Cb->onCallState(E_CALL_VIDEO_FOUND_KEYFRAME, str_acct, str_ip, n_port, ci.id, "");
	}

	// virtual void onCallRxOffer(OnCallRxOfferParam &prm);
	virtual void setHandle(long long n_revHandle, long long n_selfHandle) {
		PJ_LOG(3, (THIS_FILE, "setHandle,n_revHandle:%d, n_selfHandle:%d, %d", n_revHandle, n_selfHandle, __LINE__));
	}

private:
	void *handle;
	int n_wind_id;
	int nCapDev;
	AudioMediaPlayer* p_myPlayer;
	bool bHasStop;
};

//MyCall *call = NULL;
class DdAccount : public Account
{
public:
	virtual void onRegState(OnRegStateParam &prm)
	{
		PJ_LOG(3, (THIS_FILE, "onRegState:%d, %d, %s, %d",prm.code, prm.status, prm.reason.c_str(), __LINE__));
		AccountInfo ai = getInfo();
		if (PjsuaDDApiInterface::instance().m_pjsua2Cb != NULL) {
			PjsuaDDApiInterface::instance().m_pjsua2Cb->onRegisterState(prm.code, prm.reason);
		}
	}
	virtual void onRegStarted(OnRegStartedParam &prm)
	{
		DD_UNUSED(prm)
		AccountInfo ai = getInfo();
	}
	virtual void  onIncomingCall(OnIncomingCallParam &iprm)
	{
		PJ_LOG(3, (THIS_FILE, "onIncomingCall.id:%d,%s, iscalling,%d, %d",iprm.callId, iprm.rdata.wholeMsg.c_str(),
			PjsuaDDApiInterface::instance().isCalling(), __LINE__));
		AccountInfo ai = getInfo();
		//xqtpjsua->ui.txt_callstate->setText("onIncomingCall");
		
		if (PjsuaDDApiInterface::instance().isCalling() || PjsuaDDApiInterface::instance().m_pjsua2Cb == NULL) {

			CallOpParam oppara = CallOpParam(true);
			oppara.statusCode = PJSIP_SC_BUSY_HERE;

			pjsua_call_hangup(iprm.callId, PJSIP_SC_BUSY_HERE, NULL, NULL);
			// busyCall->answer(oppara);
			// busyCall->hangup(oppara);
			return;
		}
		std::shared_ptr<Call> busyCall = std::make_shared<DdCall>(*this, iprm.callId);

		PjsuaDDApiInterface::instance().setCalling(busyCall);

		busyCall->initTonegen(PjsuaDDApiInterface::instance().strGetInCallFile());
		// busyCall->ringStart();

		std::shared_ptr<Call> current_call = PjsuaDDApiInterface::instance().getCalling();
		std::string str_acct = "";
		std::string str_ip = "";
		int n_port = 0;
		std::string xtran = get_x_tran(iprm.rdata.wholeMsg);
		pjcall_get_remote_acc_info(current_call->getInfo().remoteUri, current_call->getInfo().remoteContact, str_acct, n_port, str_ip);
		PJ_LOG(3, (THIS_FILE, "pjcall_get_remote_acc_info.str_acct:%s, n_port:%d, str_ip:%s, remoteUri:%s, remoteContact:%s, %s, %d",
			str_acct.c_str(), n_port, str_ip.c_str(), current_call->getInfo().remoteUri.c_str(), current_call->getInfo().remoteContact.c_str(), 
			xtran.c_str(), __LINE__));
		PjsuaDDApiInterface::instance().m_pjsua2Cb->onCallState(E_INCOMMING_CALL, str_acct, str_ip, n_port, 
			current_call->getId(), get_x_tran(iprm.rdata.wholeMsg));
	}
};

PjsuaDDApiInterface& PjsuaDDApiInterface::instance() PJSUA2_THROW(Error)
{
	if (!instance_) {
		PJSUA2_RAISE_ERROR(PJ_ENOTFOUND);
	}
	return *instance_;
}


PjsuaDDApiInterface::PjsuaDDApiInterface(const pjsip_account_info &acc_info, PjsuaDDApiCb &api_cb)
	:m_pAcct(NULL), m_pCurrentCall(NULL), m_myAcctInfo(acc_info), m_pjsua2Cb(&api_cb), m_nInput(80), m_nOutput(80), bHasSound(true)
{
	if (instance_) {
		PJSUA2_RAISE_ERROR(PJ_EEXISTS);
	}

	instance_ = this;

	try {
		TransportConfig tCfg;
		m_ep.libCreate(); 
		m_epCfg.logConfig.filename = m_myAcctInfo.str_log_file;
		if (!m_myAcctInfo.str_log_file.empty()) {
			m_epCfg.logConfig.filename = m_myAcctInfo.str_log_file;
		}
		m_epCfg.medConfig.b_ice = m_myAcctInfo.b_ice;
		m_epCfg.logConfig.level = m_myAcctInfo.n_log_level;
		m_epCfg.logConfig.fileFlags = m_myAcctInfo.b_logAppend ? PJ_O_APPEND : 0;
		// m_epCfg.uaConfig.maxCalls = 1;
		m_ep.libInit(m_epCfg);

		tCfg.port = m_myAcctInfo.n_port <= 0 ? 5060: m_myAcctInfo.n_port;
		tCfg.portRange = m_myAcctInfo.n_port_range;
		TransportId tid = -1;
		if (m_myAcctInfo.n_model == PJSIP_MODEL_TCP) {
			tid = m_ep.transportCreate(PJSIP_TRANSPORT_TCP, tCfg);
		}
		else if (m_myAcctInfo.n_model == PJSIP_MODEL_TLS) {
			tid = m_ep.transportCreate(PJSIP_TRANSPORT_TLS, tCfg);
		}
		else {
			tid = m_ep.transportCreate(PJSIP_TRANSPORT_UDP, tCfg);
		}
		TransportInfo tranInfo = m_ep.transportGetInfo(tid);

		if (m_myAcctInfo.bP2p) {
			std::string localName = tranInfo.localName;

			m_myAcctInfo.str_domain = localName.substr(0, localName.find(":"));
			m_myAcctInfo.n_port = atoi(localName.substr(localName.find(":") + 1).c_str());
		}
		PJ_LOG(4, (THIS_FILE, "PjsuaDDApiInterface: str_domain:%s, port:%d, bp2p:%d, %d", m_myAcctInfo.str_domain.c_str(), 
			m_myAcctInfo.n_port, m_myAcctInfo.bP2p, __LINE__));

		AudioDevInfoVector2 audioDevList = m_ep.audDevManager().enumDev2();
		PJ_LOG(3, (THIS_FILE, "enumDev2 size:%d", audioDevList.size()));

		int inCnt = 0, outCnt = 0;
		for (auto devInfo : audioDevList) {
			PJ_LOG(3, (THIS_FILE, "name:%s, devInfo.inputCount:%d, devInfo.outputCount:%d", devInfo.name, devInfo.inputCount, devInfo.outputCount));
			inCnt += devInfo.inputCount;
			outCnt += devInfo.outputCount;
		}
		if (inCnt <= 0 || outCnt <= 0) {
			m_ep.audDevManager().setNullDev();
			bHasSound = false;
		}

		m_ep.libStart();
		//ÏÔÊ¾±àÂë
		CodecInfoVector2  audiocodecs = m_ep.codecEnum2();
		for (int i = 0; i < audiocodecs.size(); i++) {
			CodecInfo x = audiocodecs.at(i);
			std::string strCodecId = x.codecId;
			//std::transform(strCodecId.begin(), strCodecId.end(), strCodecId.begin(), ::tolower);
			if (m_myAcctInfo.b_722 && strCodecId.find("722") != std::string::npos) {
				m_ep.codecSetPriority(x.codecId, PJMEDIA_CODEC_PRIO_NEXT_HIGHER);
				continue;
			}
			if (m_myAcctInfo.b_g729 && strCodecId.find("729") != std::string::npos) {
				m_ep.codecSetPriority(x.codecId, PJMEDIA_CODEC_PRIO_HIGHEST);
				continue;
			}
			if (m_myAcctInfo.b_gsm && strCodecId.find("GSM") != std::string::npos) {
				m_ep.codecSetPriority(x.codecId, PJMEDIA_CODEC_PRIO_LOWEST);
				continue;
			}
			if (m_myAcctInfo.b_pcma && strCodecId.find("PCMA") != std::string::npos) {
				m_ep.codecSetPriority(x.codecId, PJMEDIA_CODEC_PRIO_NEXT_HIGHER);
				continue;
			}
			if (m_myAcctInfo.b_pcmu && strCodecId.find("PCMU") != std::string::npos) {
				m_ep.codecSetPriority(x.codecId, PJMEDIA_CODEC_PRIO_NEXT_HIGHER);
				continue;
			}
			m_ep.codecSetPriority(x.codecId, PJMEDIA_CODEC_PRIO_DISABLED);
		}
		CodecInfoVector2 videocodecs = m_ep.videoCodecEnum2();
		for (int i = 0; i < videocodecs.size(); i++) {
			CodecInfo x = videocodecs.at(i);
			std::string strVcodec = x.codecId;
			VidCodecParam code_para = m_ep.getVideoCodecParam(strVcodec);

			code_para.decFmt.fpsNum = m_myAcctInfo.de_fps;
			code_para.decFmt.fpsDenum = 1;
			code_para.decFmt.avgBps = m_myAcctInfo.de_avg_bps;
			code_para.decFmt.maxBps = m_myAcctInfo.de_avg_bps * 2;
			code_para.decFmt.width = m_myAcctInfo.de_width;
			code_para.decFmt.height = m_myAcctInfo.de_height;

			code_para.encFmt.fpsNum = m_myAcctInfo.en_fps;
			code_para.encFmt.fpsDenum = 1;
			code_para.encFmt.avgBps = m_myAcctInfo.en_avg_bps;
			code_para.encFmt.maxBps = m_myAcctInfo.en_avg_bps * 2;
			code_para.encFmt.width = m_myAcctInfo.en_width;
			code_para.encFmt.height = m_myAcctInfo.en_height;

			if (m_myAcctInfo.b_h264 && strVcodec.find("264") != std::string::npos) {
				m_ep.videoCodecSetPriority(x.codecId, PJMEDIA_CODEC_PRIO_HIGHEST);
				m_ep.setVideoCodecParam(strVcodec, code_para);
				continue;
			}
			if (m_myAcctInfo.b_vp8 && strVcodec.find("v8") != std::string::npos) {
				m_ep.videoCodecSetPriority(x.codecId, PJMEDIA_CODEC_PRIO_NORMAL);
				m_ep.setVideoCodecParam(strVcodec, code_para);
				continue;
			}
			if (m_myAcctInfo.b_vp9 && strVcodec.find("v9") != std::string::npos) {
				m_ep.videoCodecSetPriority(x.codecId, PJMEDIA_CODEC_PRIO_NORMAL);
				m_ep.setVideoCodecParam(strVcodec, code_para);
				continue;
			}
			m_ep.videoCodecSetPriority(x.codecId, PJMEDIA_CODEC_PRIO_DISABLED);
		}
	}
	catch (Error &err) { PJ_LOG(3, (THIS_FILE, "%s:%d", err.info().c_str(), __LINE__)); }
}

PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::registerAcct() {
	if (m_pAcct) {
		try {
			m_pAcct->setRegistration(true);
		}
		catch (Error &err) {
			PJ_LOG(3, (THIS_FILE, "PjsuaDDApiInterface::registerAcct failed, %s:%d", err.info().c_str(), __LINE__));
		}
		return E_HAS_OPERATED;
	}
	if (!m_ep.libIsThreadRegistered()) {
		m_ep.libRegisterThread("registerAcct");
	}
	m_paCfg = new AccountConfig();
	std::string idUri = "sip:" + m_myAcctInfo.str_acct + "@" + m_myAcctInfo.str_domain;
	std::string registrarUri = "sip:" + m_myAcctInfo.str_domain;
	m_paCfg->idUri = idUri;
	if (!m_myAcctInfo.bP2p){
		m_paCfg->regConfig.retryIntervalSec = m_myAcctInfo.n_retry_reg_intervel;
		m_paCfg->regConfig.timeoutSec = m_myAcctInfo.n_retry_reg_intervel;
		PJ_LOG(3, (THIS_FILE, "registerAcct n_retry_reg_intervel:%d, %d", m_myAcctInfo.n_retry_reg_intervel, __LINE__));
		//m_aCfg.regConfig.retryIntervalSec = m_myAcctInfo.n_retry_reg_intervel;
		m_paCfg->regConfig.registrarUri = registrarUri;
		AuthCredInfo cred("digest", "*", m_myAcctInfo.str_acct, 0, m_myAcctInfo.str_pwd);
		m_paCfg->sipConfig.authCreds.push_back(cred);
	}
	if (m_myAcctInfo.n_model == PJSIP_MODEL_TLS) {
		m_paCfg->sipConfig.proxies.push_back("sip:" + m_myAcctInfo.str_domain + ";hide;transport=tls");
	}

	m_paCfg->callConfig.timerMinSESec = 90;
	m_paCfg->callConfig.timerSessExpiresSec = 1800;

	m_paCfg->videoConfig.autoShowIncoming = false;
	m_paCfg->videoConfig.autoTransmitOutgoing = m_myAcctInfo.b_ignorefmtp;
	m_paCfg->videoConfig.defaultCaptureDevice = PJMEDIA_VID_DEFAULT_CAPTURE_DEV;
	m_paCfg->videoConfig.defaultRenderDevice = PJMEDIA_VID_DEFAULT_RENDER_DEV;
	m_paCfg->mediaConfig.srtpSecureSignaling = 0;
	try {
		m_pAcct = new DdAccount();
		m_pAcct->create(*m_paCfg);
	}
	catch (Error &err) { 
		if (m_pAcct != NULL) {
			delete m_pAcct;
			m_pAcct = NULL;
		}
		PJ_LOG(3, (THIS_FILE, "registerAcct failed, %s:%d", err.info().c_str(), __LINE__)); 
		return E_OTHER_ERROR;
	}
	return bHasSound ? E_SIPDD_SUCCESS : E_NO_SOUND_DEVICE;
}

PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::unregisterAcct() {
	if (m_pAcct) {
		try {
			m_pAcct->shutdown();
			if (isCalling()) {
				CallOpParam oppara;
				oppara.statusCode = PJSIP_SC_OK;
				m_pCurrentCall->hangup(oppara);
			}
			m_pCurrentCall = NULL;
			m_ep.hangupAllCalls();
			delete m_pAcct;
			delete m_paCfg;
			m_paCfg = NULL;
			m_pAcct = NULL;
		}
		catch (Error &err) {
			m_pCurrentCall = NULL;
			m_ep.hangupAllCalls();
			delete m_pAcct;
			delete m_paCfg;
			m_paCfg = NULL;
			m_pAcct = NULL;
		}

	}
	return E_SIPDD_SUCCESS;
}


PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::registerAcct(std::string strDomain, std::string strAcct, std::string strPwd) {
	if (m_pAcct) {
		try {
			m_pAcct->shutdown();
			if (isCalling()) {
				CallOpParam oppara;
				oppara.statusCode = PJSIP_SC_OK;
				m_pCurrentCall->hangup(oppara);
			}
			m_pCurrentCall = NULL;
			m_ep.hangupAllCalls();
			delete m_pAcct;
			delete m_paCfg;
			m_paCfg = NULL;
			m_pAcct = NULL;
		}
		catch (Error &err) {
			delete m_pAcct;
			delete m_paCfg;
			m_paCfg = NULL;
			m_pAcct = NULL;
			PJ_LOG(3, (THIS_FILE, "PjsuaDDApiInterface::registerAcct failed, %s:%d", err.info().c_str(), __LINE__));
		}
	}
	if (!m_ep.libIsThreadRegistered()) {
		m_ep.libRegisterThread("registerAcct");
	}
	m_myAcctInfo.str_domain = strDomain;
	m_myAcctInfo.str_acct = strAcct;
	m_myAcctInfo.str_pwd = strPwd;
	m_paCfg = new AccountConfig();
	std::string idUri = "sip:" + m_myAcctInfo.str_acct + "@" + m_myAcctInfo.str_domain;
	std::string registrarUri = "sip:" + m_myAcctInfo.str_domain;
	m_paCfg->idUri = idUri;
	if (!m_myAcctInfo.bP2p) {
		m_paCfg->regConfig.retryIntervalSec = m_myAcctInfo.n_retry_reg_intervel;
		m_paCfg->regConfig.timeoutSec = m_myAcctInfo.n_retry_reg_intervel;
		PJ_LOG(3, (THIS_FILE, "registerAcct n_retry_reg_intervel:%d, %d", m_myAcctInfo.n_retry_reg_intervel, __LINE__));
		//m_aCfg.regConfig.retryIntervalSec = m_myAcctInfo.n_retry_reg_intervel;
		m_paCfg->regConfig.registrarUri = registrarUri;
		AuthCredInfo cred("digest", "*", m_myAcctInfo.str_acct, 0, m_myAcctInfo.str_pwd);
		m_paCfg->sipConfig.authCreds.push_back(cred);
	}
	if (m_myAcctInfo.n_model == PJSIP_MODEL_TLS) {
		m_paCfg->sipConfig.proxies.push_back("sip:" + m_myAcctInfo.str_domain + ";hide;transport=tls");
	}

	m_paCfg->callConfig.timerMinSESec = 90;
	m_paCfg->callConfig.timerSessExpiresSec = 1800;

	m_paCfg->videoConfig.autoShowIncoming = false;
	m_paCfg->videoConfig.autoTransmitOutgoing = m_myAcctInfo.b_ignorefmtp;
	m_paCfg->videoConfig.defaultCaptureDevice = PJMEDIA_VID_DEFAULT_CAPTURE_DEV;
	m_paCfg->videoConfig.defaultRenderDevice = PJMEDIA_VID_DEFAULT_RENDER_DEV;
	m_paCfg->mediaConfig.srtpSecureSignaling = 0;
	try {
		m_pAcct = new DdAccount();
		m_pAcct->create(*m_paCfg);
	}
	catch (Error &err) {
		if (m_pAcct != NULL) {
			delete m_pAcct;
			m_pAcct = NULL;
		}
		PJ_LOG(3, (THIS_FILE, "registerAcct failed, %s:%d", err.info().c_str(), __LINE__));
		return E_OTHER_ERROR;
	}
	return bHasSound ? E_SIPDD_SUCCESS : E_NO_SOUND_DEVICE;
}


PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::Online() {
	if (!m_ep.libIsThreadRegistered()) {
		m_ep.libRegisterThread("Online");
	}
	if (m_pAcct == NULL || !m_pAcct->isValid()) {
		PJ_LOG(3, (THIS_FILE, "Online failed, acct id is unvalid£¡ %d", __LINE__));
		return E_NO_ACCT_ERROR;
	}
	pj_status_t status = pjsua_acc_set_online_status(m_pAcct->getId(), true);
	if (status != PJ_SUCCESS) {
		PJ_LOG(3, (THIS_FILE, "Online failed,acct id is unvalid£¡%d, %d", status, __LINE__));
		return E_NO_ACCT_ERROR;
	}
	return E_SIPDD_SUCCESS;
}

PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::Offline() {
	if (!m_ep.libIsThreadRegistered()) {
		m_ep.libRegisterThread("Offline");
	}
	if (m_pAcct == NULL || !m_pAcct->isValid()) {
		PJ_LOG(3, (THIS_FILE, "Offline failed,acct id is unvalid£¡ %d", __LINE__));
		return E_NO_ACCT_ERROR;
	}
	pj_status_t status = pjsua_acc_set_online_status(m_pAcct->getId(), false);
	if (status != PJ_SUCCESS) {
		PJ_LOG(3, (THIS_FILE, "Offline failed, acct id is unvalid£¡%d, %d", status, __LINE__));
		return E_NO_ACCT_ERROR;
	}

	return E_SIPDD_SUCCESS;
}

PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::busyCurrentCall(int n_call_id) {
	if (NULL == m_pAcct) {
		return E_NO_ACCT_ERROR;
	}
	if (!m_ep.libIsThreadRegistered()) {
		m_ep.libRegisterThread("busyCurrentCall");
	}

	if (NULL == m_pCurrentCall || m_pCurrentCall->getId() != n_call_id || !isCalling()) {
		PJ_LOG(3, (THIS_FILE, "busyCurrentCall failed, n_call_id:%d is error£¡ %d", n_call_id, __LINE__));
		return E_PARA_ERROR;
	}

	pjsua_call_hangup(n_call_id, PJSIP_SC_BUSY_HERE, NULL, NULL);
	return E_SIPDD_SUCCESS;
}

int PjsuaDDApiInterface::outCall(const std::string & str_acct, const std::string &str_ip, int n_port, const std::string &str_xtran, bool bVideo) {
	if (NULL == m_pAcct) {
		return -1;
	}
	if (!m_ep.libIsThreadRegistered()) {
		m_ep.libRegisterThread("outCall");
	}
	if (isCalling()) {
		PJ_LOG(3, (THIS_FILE, "Now is calling:%d", __LINE__));
		return -1;
	}
	m_pCurrentCall = std::make_shared<DdCall>(*m_pAcct);
	m_pCurrentCall->initTonegen(strGetOutCallFile());

	CallOpParam prm(true); // Use default call settings
	if (bVideo){
		prm.opt.videoCount = 1;
	}
	else {
		prm.opt.videoCount = 0;
	}

	if (!str_xtran.empty()) {
		SipHeader callhead;
		callhead.hName = "X-Tran";
		callhead.hValue = str_xtran;
		prm.txOption.headers.push_back(callhead);
	}
	m_pjsua2Cb->m_strXtran = str_xtran;
	n_port = n_port == 0 ? m_myAcctInfo.n_port : n_port;
	std::string stracc = "sip:" + str_acct + "@" + str_ip + ":" + std::to_string(n_port);
	try {
		m_pCurrentCall->makeCall(stracc, prm);
		m_pCurrentCall->ringStart();
	}
	catch (Error &err) { 
		PJ_LOG(3, (THIS_FILE, "outCall failed, %s:%d", err.info().c_str(), __LINE__)); 
		return -1;
	}

	return m_pCurrentCall->getId();
}

PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::hangupCurrentCall(int n_call_id, pjsip_status_code stCode) {
	std::future<PJSIP_DDAPI_STATUS_CODE> ret_future = std::async(std::launch::async, &PjsuaDDApiInterface::hangupCurrentCall_internal, this, n_call_id, stCode);
	PJ_LOG(3, (THIS_FILE, "PjsuaDDApiInterface::hangupCurrentCall:%d", __LINE__));
	return E_SIPDD_SUCCESS;
}

PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::hangupCurrentCall_internal(int n_call_id, pjsip_status_code stCode) {
	if (NULL == m_pAcct) {
		return E_NO_ACCT_ERROR;
	}
	if (!m_ep.libIsThreadRegistered()) {
		m_ep.libRegisterThread("hangupCurrentCall_internal");
	}

	if (NULL == m_pCurrentCall || (m_pCurrentCall->getId() != n_call_id && n_call_id != -1) || !isCalling()) {
		PJ_LOG(3, (THIS_FILE, "hangupCurrentCall_internal failed, n_call_id:%d is error£¡ %d", n_call_id, __LINE__));
		return E_PARA_ERROR;
	}

	CallOpParam prm = CallOpParam(true);
	prm.statusCode = stCode;
	try {
		if (n_call_id >= 0) {
			m_pCurrentCall->hangup(prm);
		}
		else {
			m_ep.hangupAllCalls();
		}
	}
	catch (Error &err) { 
		PJ_LOG(3, (THIS_FILE, "hangupCurrentCall_internal failed, %s:%d", err.info().c_str(), __LINE__)); 
		return E_OTHER_ERROR;
	}

	return E_SIPDD_SUCCESS;
}


PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::answerCurrentCall(int n_call_id, pjsip_status_code stCode) {

	if (NULL == m_pAcct) {
		return E_NO_ACCT_ERROR;
	}
	if (!m_ep.libIsThreadRegistered()) {
		m_ep.libRegisterThread("answerCurrentCall");
	}

	if (NULL == m_pCurrentCall || m_pCurrentCall->getId() != n_call_id || !isCalling()) {
		PJ_LOG(3, (THIS_FILE, "answerCurrentCall failed, n_call_id:%d is error£¡ %d", n_call_id, __LINE__));
		return E_PARA_ERROR;
	}

	CallOpParam prm = CallOpParam(true);
	prm.statusCode = stCode;
	try {
		m_pCurrentCall->ringStop();
		m_pCurrentCall->answer(prm);
	}
	catch (Error &err) { 
		PJ_LOG(3, (THIS_FILE, "answerCurrentCall failed, %s:%d", err.info().c_str(), __LINE__)); 
		return E_OTHER_ERROR;
	}

	return E_SIPDD_SUCCESS;
}

PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::enableVideo(int n_call_id, bool bVideo) {
	if (NULL == m_pAcct) {
		return E_NO_ACCT_ERROR;
	}
	if (!m_ep.libIsThreadRegistered()) {
		m_ep.libRegisterThread("enableVideo");
	}

	if (NULL == m_pCurrentCall || m_pCurrentCall->getId() != n_call_id) {
		PJ_LOG(3, (THIS_FILE, "enableVideo failed, n_call_id%d is error! %d", n_call_id, __LINE__));
		return E_PARA_ERROR;
	}
	try {
		if (isCalling()) {
			CallVidSetStreamParam streamParam;
			pjsua_call_vid_strm_op op = bVideo ? PJSUA_CALL_VID_STRM_START_TRANSMIT : PJSUA_CALL_VID_STRM_STOP_TRANSMIT;
			m_pCurrentCall->vidSetStream(op, streamParam);
		}
	}
	catch (Error &err) { 
		PJ_LOG(3, (THIS_FILE, "enableVideo failed, %s:%d", err.info().c_str(), __LINE__)); 
		return E_OTHER_ERROR;
	}

	return E_SIPDD_SUCCESS;
}

PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::muteCurrentCall(int n_call_id, bool b_mute) {
	if (!m_ep.libIsThreadRegistered()) {
		m_ep.libRegisterThread("muteCurrentCall");
	}
	if (NULL == m_pAcct) {
		return E_NO_ACCT_ERROR;
	}

	if (NULL == m_pCurrentCall || m_pCurrentCall->getId() != n_call_id || !isCalling()) {
		PJ_LOG(3, (THIS_FILE, "muteCurrentCall failed, n_call_id:%d is error£¡ %d", n_call_id, __LINE__));
		return E_PARA_ERROR;
	}
	AudioMedia aMediao = m_pCurrentCall->getAudioMedia(-1);
	if (b_mute) {
		pj_status_t status = pjsua_conf_disconnect(0, aMediao.getPortId());
	}
	else {
		pj_status_t status = pjsua_conf_connect(0, aMediao.getPortId());
	}
	
	//
	//aMediao.stopTransmit();
	return E_SIPDD_SUCCESS;
}

PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::capturePicture(int n_call_id, const std::string& strFileName) {
	DD_UNUSED(n_call_id);
	DD_UNUSED(strFileName);
	return E_SIPDD_SUCCESS;
}

std::string PjsuaDDApiInterface::getStreamQuality(int n_call_id)
{
	if (!m_ep.libIsThreadRegistered()) {
		m_ep.libRegisterThread("getStreamQuality");
	}
	if (NULL == m_pAcct) {
		return "";
	}

	if (NULL == m_pCurrentCall || m_pCurrentCall->getId() != n_call_id || !isCalling()) {
		PJ_LOG(3, (THIS_FILE, "getStreamQuality failed, n_call_id:% is error£¡ %d", n_call_id, __LINE__));
		return "";
	}
	try {
		return m_pCurrentCall->dump(true, "  ");
	}
	catch (Error &err) {
		PJ_LOG(3, (THIS_FILE, "getStreamQuality failed, %s:%d", err.info().c_str(), __LINE__));
	}
	return "";
}

PjsuaDDApiInterface::~PjsuaDDApiInterface() {
	if (!m_ep.libIsThreadRegistered()) {
		m_ep.libRegisterThread("busyCurrentCall");
	}
	try {
		if (isCalling()) {
			CallOpParam oppara;
			oppara.statusCode = PJSIP_SC_NULL;
			m_pCurrentCall->hangup(oppara);
		}
		m_pCurrentCall = NULL;
		m_ep.hangupAllCalls();
		delete m_pAcct;
		m_pAcct = NULL;
	}
	catch (Error &err) {
		PJ_LOG(3, (THIS_FILE, "PjsuaDDApiInterface failed, %s:%d", err.info().c_str(), __LINE__));
	}
}


bool PjsuaDDApiInterface::isCalling() {
	if (m_pCurrentCall != NULL && m_pCurrentCall->isActive()) {
		return true;
	}
	return false;
}


unsigned PjsuaDDApiInterface::getOutputVolume() {

	return m_nOutput;
}

PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::setOutputVolume(unsigned volume) {
	pjsua_conf_adjust_tx_level(0, volume);
	return E_SIPDD_SUCCESS;
}

unsigned PjsuaDDApiInterface::getInputVolume() {
	return m_nInput;
}

PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::setInputVolume(unsigned volume) {
	m_nInput = volume; 
	return E_SIPDD_SUCCESS;
}

PJSIP_DDAPI_STATUS_CODE PjsuaDDApiInterface::incommingCallStartRinging(int n_call_id){
		if (NULL == m_pAcct) {
		return E_NO_ACCT_ERROR;
	}
	if (!m_ep.libIsThreadRegistered()) {
		m_ep.libRegisterThread("enableVideo");
	}

	if (NULL == m_pCurrentCall || m_pCurrentCall->getId() != n_call_id) {
		PJ_LOG(3, (THIS_FILE, "enableVideo failed, n_call_id%d is error! %d", n_call_id, __LINE__));
		return E_PARA_ERROR;
	}
	try {
		if (isCalling()) {
			m_pCurrentCall->ringStart();
		}
	}
	catch (Error &err) {
		PJ_LOG(3, (THIS_FILE, "PjsuaDDApiInterface failed, %s:%d", err.info().c_str(), __LINE__));
	}
	return E_SIPDD_SUCCESS;
}