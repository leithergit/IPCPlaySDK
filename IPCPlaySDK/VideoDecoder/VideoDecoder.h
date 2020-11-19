#pragma once
#include <windows.h>
#include <tchar.h>
#include <d3d9.h>
#include <dxva2api.h>
#include <assert.h>
#include <initguid.h>
#include <dvdmedia.h>
#include <dmodshow.h>
#include <MMReg.h>
#include <wmcodecdsp.h>
#include <Vfw.h>

//#include "moreuuids.h"
#include "IHWVideo_Typedef.h"
#include "IHW265Dec_Api.h"
#include "hi_config.h"
#include <VersionHelpers.h>


#ifndef IN
#define	IN
#endif

#ifndef OUT
#define	OUT
#endif

#ifndef INOUT
#define	INOUT
#endif
#pragma warning(disable:4244 4018)
#pragma warning(push)

#ifdef __cplusplus
extern "C" {
#endif
#define __STDC_CONSTANT_MACROS
#define FF_API_PIX_FMT 0
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavcodec/dxva2.h"
#include "libavutil/pixdesc.h"
#include "libavutil/pixfmt.h"	

	//#define _ENABLE_FFMPEG_STAITC_LIB

#ifdef _ENABLE_FFMPEG_STAITC_LIB
#pragma comment(lib,"libgcc.a")
//#pragma comment(lib,"libmingwex.a")
//#pragma comment(lib,"libcoldname.a")
#pragma comment(lib,"libavcodec.a")
#pragma comment(lib,"libavformat.a")
#pragma comment(lib,"libavutil.a")
#pragma comment(lib,"libswscale.a")
#pragma comment(lib,"libswresample.a")
#pragma comment(lib,"WS2_32.lib")	
#else
#pragma comment(lib,"avcodec.lib")
	//#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"swscale.lib")	
#endif	
#ifdef __cplusplus
}
#endif
#pragma warning(pop)

#include "../DxSurface/DxTrace.h"
#include "../AutoLock.h"
#include <string>
using namespace std;

enum Decoder_Manufacturer
{
	FFMPEG = 0,
	HISILICON = 1,
};

#ifdef INITGUID
#define DEFINE_GUID_S(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define DEFINE_GUID_S(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID FAR name
#endif // INITGUID

DEFINE_GUID_S(DXVA_ModeH264_VLD_WithFMOASO_NoFGT, 0xd5f04ff9, 0x3418, 0x45d8, 0x95, 0x61, 0x32, 0xa7, 0x6a, 0xae, 0x2d, 0xdd);
DEFINE_GUID_S(DXVA_ModeH264_VLD_NoFGT_Flash, 0x4245F676, 0x2BBC, 0x4166, 0xa0, 0xBB, 0x54, 0xE7, 0xB8, 0x49, 0xC3, 0x80);

// Intel ClearVideo H264 decoders
DEFINE_GUID_S(DXVADDI_Intel_ModeH264_A, 0x604F8E64, 0x4951, 0x4c54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);
DEFINE_GUID_S(DXVADDI_Intel_ModeH264_C, 0x604F8E66, 0x4951, 0x4c54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);
DEFINE_GUID_S(DXVADDI_Intel_ModeH264_E, 0x604F8E68, 0x4951, 0x4C54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);

// Intel ClearVideo VC1 bitstream decoder
DEFINE_GUID_S(DXVA_Intel_VC1_ClearVideo, 0xBCC5DB6D, 0xA2B6, 0x4AF0, 0xAC, 0xE4, 0xAD, 0xB1, 0xF7, 0x87, 0xBC, 0x89);
DEFINE_GUID_S(DXVA_Intel_VC1_ClearVideo_2, 0xE07EC519, 0xE651, 0x4CD6, 0xAC, 0x84, 0x13, 0x70, 0xCC, 0xEE, 0xC8, 0x51);


// MPEG4-ASP
DEFINE_GUID_S(DXVA_nVidia_MPEG4_ASP, 0x9947EC6F, 0x689B, 0x11DC, 0xA3, 0x20, 0x00, 0x19, 0xDB, 0xBC, 0x41, 0x84);
DEFINE_GUID_S(DXVA_ModeMPEG4pt2_VLD_Simple, 0xefd64d74, 0xc9e8, 0x41d7, 0xa5, 0xe9, 0xe9, 0xb0, 0xe3, 0x9f, 0xa3, 0x19);
DEFINE_GUID_S(DXVA_ModeMPEG4pt2_VLD_AdvSimple_NoGMC, 0xed418a9f, 0x10d, 0x4eda, 0x9a, 0xe3, 0x9a, 0x65, 0x35, 0x8d, 0x8d, 0x2e);
DEFINE_GUID_S(DXVA_ModeMPEG4pt2_VLD_AdvSimple_GMC, 0xab998b5b, 0x4258, 0x44a9, 0x9f, 0xeb, 0x94, 0xe5, 0x97, 0xa6, 0xba, 0xae);
DEFINE_GUID_S(DXVA_ModeMPEG4pt2_VLD_AdvSimple_Avivo, 0x7C74ADC6, 0xe2ba, 0x4ade, 0x86, 0xde, 0x30, 0xbe, 0xab, 0xb4, 0x0c, 0xc1);

// DXVA2 MVC modes
DEFINE_GUID_S(DXVA_ModeH264_VLD_Stereo_Progressive_NoFGT, 0xd79be8da, 0xcf1, 0x4c81, 0xb8, 0x2a, 0x69, 0xa4, 0xe2, 0x36, 0xf4, 0x3d);
DEFINE_GUID_S(DXVA_ModeH264_VLD_Stereo_NoFGT, 0xf9aaccbb, 0xc2b6, 0x4cfc, 0x87, 0x79, 0x57, 0x7, 0xb1, 0x76, 0x5, 0x52);
DEFINE_GUID_S(DXVA_ModeH264_VLD_Multiview_NoFGT, 0x705b9d82, 0x76cf, 0x49d6, 0xb7, 0xe6, 0xac, 0x88, 0x72, 0xdb, 0x1, 0x3c);

// DXVA2 SVC modes
DEFINE_GUID_S(DXVA_ModeH264_VLD_SVC_Scalable_Baseline, 0xc30700c4, 0xe384, 0x43e0, 0xb9, 0x82, 0x2d, 0x89, 0xee, 0x7f, 0x77, 0xc4);
DEFINE_GUID_S(DXVA_ModeH264_VLD_SVC_Restricted_Scalable_Baseline, 0x9b8175d4, 0xd670, 0x4cf2, 0xa9, 0xf0, 0xfa, 0x56, 0xdf, 0x71, 0xa1, 0xae);
DEFINE_GUID_S(DXVA_ModeH264_VLD_SVC_Scalable_High, 0x728012c9, 0x66a8, 0x422f, 0x97, 0xe9, 0xb5, 0xe3, 0x9b, 0x51, 0xc0, 0x53);
DEFINE_GUID_S(DXVA_ModeH264_VLD_SVC_Restricted_Scalable_High_Progressive, 0x8efa5926, 0xbd9e, 0x4b04, 0x8b, 0x72, 0x8f, 0x97, 0x7d, 0xc4, 0x4c, 0x36);

// HEVC
DEFINE_GUID_S(DXVA_ModeHEVC_VLD_Main, 0x5b11d51b, 0x2f4c, 0x4452, 0xbc, 0xc3, 0x9, 0xf2, 0xa1, 0x16, 0xc, 0xc0);
DEFINE_GUID_S(DXVA_ModeHEVC_VLD_Main10, 0x107af0e0, 0xef1a, 0x4d19, 0xab, 0xa8, 0x67, 0xa1, 0x63, 0x7, 0x3d, 0x13);


#define FOURCC_NV12 mmioFOURCC('N','V','1','2')
//#define FOURCC_NV21 mmioFOURCC('N','V','2','1')
#define FOURCC_P010 mmioFOURCC('P','0','1','0')
#define FOURCC_P016 mmioFOURCC('P','0','1','6')


#define DXVA2_MAX_SURFACES 64
#define DXVA2_QUEUE_SURFACES 4
#define DXVA2_SURFACE_BASE_ALIGN 16

#ifndef SAFE_ARRAY_DELETE
#define SAFE_ARRAY_DELETE(x) if (x) { delete [] x; x = nullptr; }
#endif

#ifndef SafeDelete
#define SafeDelete(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif

#ifndef SafeDeleteArray
#define SafeDeleteArray(p)  { if(p) { delete[] (p);   (p)=NULL; } }
#endif

#ifndef SafeRelease
#define SafeRelease(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

// some common macros
#define Safe_Delete(pPtr) { if (pPtr) delete pPtr; pPtr = nullptr; }
#define SAFE_CO_FREE(pPtr) { CoTaskMemFree(pPtr); pPtr = nullptr; }
#define CHECK_HR(hr) if (FAILED(hr)) { goto done; }
#define QI(i) (riid == __uuidof(i)) ? GetInterface((i*)this, ppv) :
#define QI2(i) (riid == IID_##i) ? GetInterface((i*)this, ppv) :

std::wstring WStringFromGUID(const GUID& guid);
#define CheckPointer(p,ret) {if((p)==NULL) return (ret);}

typedef HRESULT WINAPI pCreateDeviceManager9(UINT *pResetToken, IDirect3DDeviceManager9 **);

typedef struct
{
	int index;
	bool used;
	LPDIRECT3DSURFACE9 pSurface;
	uint64_t age;
} d3d_surface_t;


#define AVCODEC_MAXHREADS 16

// SafeRelease Template, for type safety
// template <class T> 
// void SafeRelease(T **ppT)
// {
// 	if (*ppT)
// 	{
// 		(*ppT)->Release();
// 		*ppT = nullptr;
// 	}
// }

typedef IDirect3D9* WINAPI pD3DCreate9(UINT);
typedef HRESULT WINAPI pD3DCreate9Ex(UINT, IDirect3D9Ex**);

typedef void(*CopyFrameProc)(const BYTE *pSourceData, BYTE *pY, BYTE *pUV, size_t surfaceHeight, size_t imageHeight, size_t pitch);

extern CopyFrameProc CopyFrameNV12;
extern CopyFrameProc CopyFrameYUV420P;

/// @brief 自动注册FFmpeg解码库类
class CAvRegister
{
public:
	CAvRegister()
	{
		// av_register_all();
		// 去除注释可设置FFMPEG的日志输出回调，便于观看日志
		// av_log_set_callback(ff_log_callback);	
	}
	static void ff_log_callback(void*avcl, int level, const char*fmt, va_list vl)
	{
		char log[1024];
		vsnprintf(log, sizeof(log), fmt, vl);
		OutputDebugStringA(log);
	}
	~CAvRegister()
	{

	}
};

// @brief 用于码流探测的视频码流队列结构
struct AvQueue
{
	AvQueue(void *popaque,int nBufferSize)
	{
		pUserData = popaque;
		pAvBuffer = (uint8_t *)av_malloc(nBufferSize);
#ifdef _DEBUG
		ZeroMemory(pAvBuffer, nBufferSize);
		DxTraceMsg("%s pAvBuffer = %08X.\n", __FUNCTION__, (long)pAvBuffer);
#endif
	}
	~AvQueue()
	{
		//av_free(pAvBuffer);
		pAvBuffer = nullptr;
	}
	void	*pUserData;
	uint8_t *pAvBuffer;
};

/// @beief 视频解码类 主要封装了FFmpeg的解码库和华为海思H.265软解库
class CVideoDecoder
{
public:
	CVideoDecoder(void);

#ifdef __HisiLicon
	static void *HW265D_Malloc(UINT32 channel_id, UINT32 size)
	{
		return (void *)malloc(size);
	}
	
	static void HW265D_Free(UINT32 channel_id, void * ptr)
	{
		free(ptr);
	}

	static void HW265D_Log(UINT32 channel_id, IHWVIDEO_ALG_LOG_LEVEL eLevel, INT8 *p_msg, ...)
	{
	}

	static inline void HisiconH265Frame2FFmpegAvFrame(IH265DEC_OUTARGS *pHiFrame, AVFrame *pAvFrame)
	{
		pAvFrame->width = pHiFrame->uiDecWidth;
		pAvFrame->height = pHiFrame->uiDecHeight;
		pAvFrame->data[0] = pHiFrame->pucOutYUV[0];
		pAvFrame->data[1] = pHiFrame->pucOutYUV[1];
		pAvFrame->data[2] = pHiFrame->pucOutYUV[2];
		pAvFrame->linesize[0] = pHiFrame->uiYStride;
		pAvFrame->linesize[1] = pHiFrame->uiUVStride;
		pAvFrame->linesize[2] = pHiFrame->uiUVStride;
		pAvFrame->format = AV_PIX_FMT_YUV420P;
	}
	static inline void HisiconError(int nError, CHAR *szErrormsg, int nErrorMsgSize)
	{
		switch (nError)
		{
		case IHW265D_OK:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("Succeed."));
		}
		break;
		case IHW265D_NEED_MORE_BITS:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("Call ok, but need more bit for a frame."));
		}
		break;
		case IHW265D_FIND_NEW_PIC:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("Find a New Picture."));
			break;
		}
#if OUTPUTEST_ARM
		case IHW265D_NEED_CHANGE_SIZE:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("Need change size."));
		}
		break;
#endif
		// Warning return code
		case IHW265D_NAL_HEADER_WARNING:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("Nal header warning."));
		}
		break;
		case IHW265D_VPS_WARNING:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("vps warning."));
		}
		break;
		case IHW265D_SPS_WARNING:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("sps warning."));
		}
		break;
		case IHW265D_PPS_WARNING:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("pps warning."));
		}
		break;
		case IHW265D_SLICEHEADER_WARNING:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("sliceheader warning."));
		}
		break;
		case IHW265D_SLICEDATA_WARNING:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("slice data warning."));
		}
		break;
		// General error
		case IHW265D_INVALID_ARGUMENT:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("Input parameter is wrong."));
		}
		break;
		case IHW265D_DECODER_NOT_CREATE:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("Decoder is not Created."));
		}
		break;
		case IHW265D_MALLOC_FAIL:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("memory malloc failed."));
		}
		break;
		case IHW265D_INVALID_MAX_WIDTH:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("maximum width exceed limit."));
		}
		break;
		case IHW265D_INVALID_MAX_HEIGHT:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("maximum height exceed limit."));
		}
		break;
		case IHW265D_INVALID_MAX_REF_PIC:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("maximum reference num exceed limit."));
		}
		break;
		case IHW265D_INVALID_THREAD_CONTROL:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("error thread control."));
		}
		break;
		case IHW265D_INVALID_MALLOC_FXN:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("malloc callback function pointer invalid."));
		}
		break;
		case IHW265D_INVALID_FREE_FXN:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("free callback function pointer invalid."));
		}
		break;
		case IHW265D_INVALID_LOG_FXN:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("log callback function pointer invalid."));
		}
		break;
		case IHW265D_STREAMBUF_NULL:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("decoder input stram buf is empty."));
		}
		break;
		case IHW265D_INVALID_STREAMBUF_LENGTH:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("decoder input stream lenth error."));
		}
		break;
		case IHW265D_YUVBUF_NULL:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("decoder output yuv buffer pointer is NULL."));
		}
		break;
		case IHW265D_YUVBUF_ADDR_NOT_ALIGN_16:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("decoder output yuv buffer address not alignment by 16 byte."));
		}
		break;
		case IHW265D_POSTPROCESS_ERR:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("postprocess select error."));
		}
		break;
		case IHW265D_ERRCONCEAL_ERR:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("error canceal parameter config error."));
		}
		break;
		case IHW265D_NAL_HEADER_ERR:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("Nal Header error."));
		}
		break;
		case IHW265D_VPS_ERR:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("vps decode error.."));
		}
		break;
		case IHW265D_SPS_ERR:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("sps decode error."));
		}
		break;
		case IHW265D_PPS_ERR:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("pps decode error."));
		}
		break;
		case IHW265D_SLICEHEADER_ERR:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("sliceheader decode error."));
		}
		break;
		case IHW265D_SLICEDATA_ERR:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("slicedata decode error."));
		}
		break;
		// Decode warning
		case IHW265D_FRAME_DECODE_WARN:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("frame data warning, the stream may has error code, output yuv picture quality can not be provided."));
		}
		break;
		case IHW265D_THREAD_ERROR:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("multi thread error."));
		}
		break;
		default:
		{
			strcpy_s(szErrormsg, nErrorMsgSize, ("unkonwn error."));
		}
		break;
		}

	}
#endif
	virtual ~CVideoDecoder(void);

	/// @brief			码流探测
	/// @param [in]		read_packet		读取码流回调函数
	/// @retval			0	操作成功
	/// @retval			-1	操作失败	
	/// @remark			用于探测码流的类型，尺寸等信息

	int ProbeStream(void *Opaque,int(*read_packet)(void *opaque, uint8_t *buf, int buf_size), int nFrameBufferSize =1024*512 )
	{
		int nAvError = 0;
		char szAvError[1024] = { 0 };
		int nWriteable = 0;
		uint8_t *pAvBuffer = nullptr;
		bool bSucceed = false;
		
		if (!m_pFormatCtx)
			m_pFormatCtx = avformat_alloc_context();
		
		if (!m_pAvQueue)
		{
			m_pAvQueue = new AvQueue(Opaque, nFrameBufferSize);
		}
		
		if (m_pIoContext)
			av_free(m_pIoContext);
		m_pIoContext = avio_alloc_context(m_pAvQueue->pAvBuffer, nFrameBufferSize, nWriteable, m_pAvQueue, read_packet, nullptr, nullptr);
		if (!m_pIoContext)
		{
			DxTraceMsg("%s avio_alloc_context Failed.\n", __FUNCTION__);
			return -1;
		}
		//m_pAvQueue->pAvBuffer = pAvBuffer;
		m_pFormatCtx->pb = m_pIoContext;
		AVInputFormat *pInputFormatCtx = NULL;
		if (nAvError = av_probe_input_buffer(m_pIoContext, &pInputFormatCtx, "", NULL, 0, 0) < 0)
		{
			av_strerror(nAvError, szAvError, 1024);
			DxTraceMsg("%s av_probe_input_buffer Failed:%s\n", __FUNCTION__, szAvError);
			return nAvError;
		}

		if (nAvError = avformat_open_input(&m_pFormatCtx, "", pInputFormatCtx, nullptr) < 0)
		{
			av_strerror(nAvError, szAvError, 1024);
			DxTraceMsg("%s avformat_open_input Failed:%s\n", __FUNCTION__, szAvError);
			return nAvError;
		}

		if (nAvError = avformat_find_stream_info(m_pFormatCtx, nullptr) < 0)
		{
			av_strerror(nAvError, szAvError, 1024);
			DxTraceMsg("%s avformat_find_stream_info Failed:%s.\n", __FUNCTION__,szAvError);
			return nAvError;
		}
		for (UINT i = 0; i < m_pFormatCtx->nb_streams; i++)
		{
			if ((m_pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) &&
				(m_nVideoIndex < 0))
			{
				m_nVideoIndex = i;
			}
			if ((m_pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) &&
				(m_nAudioIndex < 0))
			{
				m_nAudioIndex = i;
			}
		}

		if (m_nVideoIndex < 0 /*|| m_nAudioIndex < 0*/)
		{
			DxTraceMsg("%s can't found any video stream or audio stream.\n", __FUNCTION__);
			return false;
		}

		m_nCodecId = m_pFormatCtx->streams[m_nVideoIndex]->codecpar->codec_id;
		AVCodec*  pAvCodec = avcodec_find_decoder(m_nCodecId);
		if (pAvCodec == NULL)
		{
			DxTraceMsg("%s avcodec_find_decoder Failed.\n", __FUNCTION__);
			//assert(false);
			return false;
		}
		
		//avcodec_parameters_to_context(AVCodecContext* codec, AVCodecParameters const* par)
		m_pAVCtx = avcodec_alloc_context3(pAvCodec);
		if (m_pAVCtx)
			avcodec_parameters_to_context(m_pAVCtx, m_pFormatCtx->streams[m_nVideoIndex]->codecpar);
		else
		{
			DxTraceMsg("%s avcodec_alloc_context3 Failed.\n", __FUNCTION__);
			return false;
		}
		m_bProbeSucceed = true;
		//m_pAVCtx = m_pFormatCtx->streams[m_nVideoIndex]->codecpar;
		return 0;
	}

	void CancelProbe()
	{
		if (m_pAvQueue)
		{
// 			if (m_pAvQueue->pAvBuffer)
// 				delete []m_pAvQueue->pAvBuffer;
			delete m_pAvQueue;
			m_pAvQueue = nullptr;
			m_pIoContext->opaque = nullptr;
		}
		if (m_pIoContext)
		{
			av_free(m_pIoContext);
			m_pIoContext = nullptr;
		}
		if (m_pFormatCtx)
			m_pFormatCtx->pb = nullptr;
	}

	void SetD3DShared(IDirect3D9Ex  *pD3D, IDirect3DDevice9Ex *pD3DDev)
	{
		if (!pD3D || !pD3DDev)
		{
			assert(false);
			return;
		}
		m_pD3D = pD3D;
		m_pD3DDev = pD3DDev;
		m_bD3DShared = true;
	}
	/// @brief 设置解码线程数量，一情况下建议使用单线程解码，只有在那些CPU性能较低的机器上使用多线程解码
	void SetDecodeThreads(int nThreads = 1)
	{
		m_nDecodeThreadCount = nThreads;
	}
	/// @brief 初始化解码器
	/// @brief 不可与LoadDecodingFile函数同时调用，二者只能选一
	/// 当nCodec不为AV_CODEC_ID_NONE时，nWidth和nHeight不可为0
	bool InitDecoder(AVCodecID nCodec = AV_CODEC_ID_NONE, int nWidth = 0,int nHeight = 0,bool bEnableHaccel = false)
	{
		AVCodecID nCodecID = nCodec;
		AVCodec*  pAvCodec = nullptr;
		if (!m_bProbeSucceed)
			DestroyDecoder();
		if (nCodec != AV_CODEC_ID_NONE /*&& nWidth && nHeight*/)
		{
			pAvCodec = avcodec_find_decoder(nCodec);
			if (!pAvCodec)
			{
				DxTraceMsg("%s avcodec_find_decoder Failed.\n", __FUNCTION__);
				//assert(false);
				return false;
			}
			m_pAVCtx = avcodec_alloc_context3(pAvCodec);
			if (!m_pAVCtx)
			{
				DxTraceMsg("%s avcodec_alloc_context3 Failed.\n", __FUNCTION__);
				return false;
			}
			m_pAVCtx->flags = 0;
// 			m_pAVCtx->time_base.num = 1; //这两行：一秒钟25帧
// 			m_pAVCtx->time_base.den = fps; // 25;
			//142*6=852
			m_pAVCtx->bit_rate = 0; //初始化为0
			m_pAVCtx->frame_number = 1; //每包一个视频帧
			m_pAVCtx->codec_type = AVMEDIA_TYPE_VIDEO;// CODEC_TYPE_VIDEO;
			m_pAVCtx->width = nWidth; //这两行：视频的宽度和高度
			m_pAVCtx->height = nHeight;
		}
		else
		{
			if (m_nVideoIndex == -1)
			{
				for (UINT i = 0; i < m_pFormatCtx->nb_streams; i++)
				{
					if ((m_pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) &&
						(m_nVideoIndex < 0))
					{
						m_nVideoIndex = i;
					}
					if ((m_pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) &&
						(m_nAudioIndex < 0))
					{
						m_nAudioIndex = i;
					}
				}

				if (m_nVideoIndex < 0 && m_nAudioIndex < 0)
				{
					DxTraceMsg("%s can't found any video stream or audio stream.\n", __FUNCTION__);
					//assert(false);
					return false;
				}
		
				nCodecID = m_pFormatCtx->streams[m_nVideoIndex]->codecpar->codec_id;
				pAvCodec = avcodec_find_decoder(nCodecID);
				if (pAvCodec == NULL)
				{
					DxTraceMsg("%s avcodec_find_decoder Failed.\n", __FUNCTION__);
					//assert(false);
					return false;
				}
				m_pAVCtx = avcodec_alloc_context3(pAvCodec);
				if (!m_pAVCtx)
					avcodec_parameters_to_context(m_pAVCtx, m_pFormatCtx->streams[m_nVideoIndex]->codecpar);
				else
				{
					DxTraceMsg("%s avcodec_alloc_context3 Failed.\n", __FUNCTION__);
					return false;
				}
			}
		}

		if (bEnableHaccel)
		{
			UINT nAdapter = D3DADAPTER_DEFAULT;
			m_dwSurfaceWidth = nWidth;
			m_dwSurfaceHeight = nHeight;
			HRESULT hr = S_OK;
			if (m_pD3D && m_pD3DDev)
				hr = InitDxva(m_pD3D, m_pD3DDev);
			else
				hr = InitD3D(nAdapter);
			if (FAILED(hr))
			{
				DxTraceMsg("D3D Initialization failed with hr: %X\n", hr);
				//assert(false);
				return false;
			}
			// 检查是否支持硬解码
			if (!CodecIsSupported(nCodecID))
			{
				bEnableHaccel = false;
				FreeD3DResources();
				if (nCodecID == AV_CODEC_ID_HEVC)
				{
					m_nManufacturer = HISILICON;
					bEnableHaccel = false;
				}
			}
		}
		
		if (m_nManufacturer == FFMPEG)
		{
			if (InitFFmpegDecoder(bEnableHaccel) == 0)
				return true;
			else
			{
				//assert(false);
				return false;
			}
		}
#ifdef __HisiLicon
		else
		{
			if (InitHisiliconDecoder() == IHW265D_OK)
				return true;
			else
			{
				//assert(false);
				return false;
			}
		}
		m_bInInit = TRUE;
#endif
		return true;
	}


private:
	/// @brief 初始化FFMPEG解码器
	int InitFFmpegDecoder(bool bEnableHaccel = false)
	{
		if (!m_pAVCtx)
		{
			//assert(false);
			return -1;
		}

		int nAvError = 0;
		char szAvError[1024] = { 0 };		
				
		if (m_nDecodeThreadCount == 0)
		{
			// Setup threading
			// Thread Count. 0 = auto detect
			int thread_count = av_cpu_count() * 3 / 2;
			m_pAVCtx->thread_count = max(1, min(thread_count, AVCODEC_MAXHREADS));
			if (m_pAVCtx->codec_id == AV_CODEC_ID_MPEG4)
			{
				m_pAVCtx->thread_count = 1;
			}
		}
		else
			m_pAVCtx->thread_count = m_nDecodeThreadCount;
		
		m_pFrame = av_frame_alloc();
		if (bEnableHaccel)
			AdditionaDecoderInit();
		m_pAVCodec = avcodec_find_decoder(m_pAVCtx->codec_id);
		if (m_pAVCodec == NULL)
		{
			DxTraceMsg("%s avcodec_find_decoder Failed.\n", __FUNCTION__);
			//assert(false);
			return -1;
		}
		nAvError = avcodec_open2(m_pAVCtx, m_pAVCodec, nullptr);
		m_bInInit = FALSE;
		if (nAvError >= 0)
		{
			m_nCodecId = m_pAVCodec->id;
		}
		else
		{
			av_strerror(nAvError, szAvError, 1024);
			DxTraceMsg("%s codec failed to avcodec_open2:\n", __FUNCTION__, szAvError);
			DestroyDecoder();
			//assert(false);
			return -1;
		}
		return 0;
	}
	/// @brief 销毁FFmpeg解码器
	int DestroyFFmpegDecoder()
	{
		m_pAVCodec = nullptr;
		if (m_pAVCtx)
		{
			DxTraceMsg("%s Shutting down ffmpeg...\n", __FUNCTION__);
			if (m_pAVCtx->hwaccel_context)
			{
				av_free(m_pAVCtx->hwaccel_context);
				m_pAVCtx->hwaccel_context = nullptr;
			}
			avcodec_close(m_pAVCtx);
			if (m_pAVCtx->extradata)
				av_freep(&m_pAVCtx->extradata);
			av_freep(&m_pAVCtx);
		}
// 		if (m_pFormatCtx)
// 		{
// 			avformat_close_input(&m_pFormatCtx);
// 			avformat_free_context(m_pFormatCtx);
// 			m_pFormatCtx = nullptr;
// 		}
// 		if (m_pIoContext)
// 		{
// 			av_free(m_pIoContext);
// 			m_pIoContext = nullptr;
// 		}
		if (m_pAvBuffer)
		{
			av_free(m_pAvBuffer);
			m_pAvBuffer = nullptr;
			m_nAvBufferSize = 0;
		}
		av_frame_free(&m_pFrame);
		m_pFrame = nullptr;
		m_nCodecId = AV_CODEC_ID_NONE;
		return 0;
	}
#ifdef __HisiLicon
	/// 初始化华为海思H.265解码器
	int InitHisiliconDecoder()
	{
		if (!m_pAVCtx)
		{
			//assert(false);
			return -1;
		}
		INT32 iRet = 0;
		IHWVIDEO_ALG_VERSION_STRU stVersion;
		if (IHW265D_OK == IHW265D_GetVersion(&stVersion))
		{
			DxTraceMsg("Version: %s\nReleaseTime %s\n\n", stVersion.cVersionChar, stVersion.cReleaseTime);
		}
		/*create decode handle*/
		m_stInitParam.uiChannelID = 0;
		m_stInitParam.iMaxWidth = m_pAVCtx->width;
		m_stInitParam.iMaxHeight = m_pAVCtx->height;
		m_stInitParam.iMaxRefNum = 4;
		m_stInitParam.eThreadType = IH265D_MULTI_THREAD;
		m_stInitParam.eOutputOrder = IH265D_DISPLAY_ORDER;
		m_stInitParam.MallocFxn = HW265D_Malloc;
		m_stInitParam.FreeFxn = HW265D_Free;
		m_stInitParam.LogFxn = HW265D_Log;

		iRet = IHW265D_Create(&m_hDecoder265, &m_stInitParam);
		if (IHW265D_OK != iRet)
		{
			DxTraceMsg("Unable to create Hisilicon H.265 decoder.\n");
			return iRet;
		}
		return 0;
	}
	/// @brief 销毁华为海思H.265解码器
	int DestroyHisiliconDecoder()
	{
		if (m_hDecoder265)
		{
			DxTraceMsg("%s Shutting down HisiliconDecoder...\n", __FUNCTION__);
			IHW265D_Delete(m_hDecoder265);
			m_hDecoder265 = nullptr;
		}
		return 0;
	}
#endif
public:
	STDMETHODIMP_(long) GetBufferCount()
	{
		long buffers = 0;

		// Native decoding should use 16 buffers to enable seamless codec changes
		// Buffers based on max ref frames
		if (m_nCodecId == AV_CODEC_ID_H264)
			buffers = 4;
		else if (m_nCodecId == AV_CODEC_ID_HEVC)
			buffers = 4;
		else
			buffers = 8;

		// 4 extra buffers for handling and safety
		//buffers += 4;
		// buffers += m_DisplayDelay;

		return buffers;
	}

	/// @brief 加载要解码的文件，同时初始化解码器
	/// @remark不可与InitDecoder同时调用，二者只能选一
	bool LoadDecodingFile(char *szFilePath, bool bEnableHaccel = false)
	{
		UINT nAdapter = D3DADAPTER_DEFAULT;
		//av_register_all();

		DestroyDecoder();
		//m_pAVCtx = avcodec_alloc_context3(m_pAVCodec);	
		char szAvError[1024] = { 0 };
		int nAvError = 0;
		m_pFormatCtx = avformat_alloc_context();
		if ((nAvError = avformat_open_input(&m_pFormatCtx, szFilePath, NULL, NULL)))
		{
			av_strerror(nAvError, szAvError, 1024);
			DxTraceMsg("%s avformat_open_input failed:%s.\n", __FUNCTION__, szAvError);
			return false;
		}
		if ((nAvError = avformat_find_stream_info(m_pFormatCtx, NULL)) < 0)
		{
			av_strerror(nAvError, szAvError, 1024);
			DxTraceMsg("%s avformat_find_stream_info failed:%s.\n", __FUNCTION__, szAvError);
			return false;
		}
		m_nVideoIndex = -1;
		int i = 0;
		for (i = 0; i < m_pFormatCtx->nb_streams; i++)
			if (m_pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				m_nVideoIndex = i;
				break;
			}

		
		m_pAVCtx = avcodec_alloc_context3(m_pAVCodec);
		if (!m_pAVCtx)
			avcodec_parameters_to_context(m_pAVCtx, m_pFormatCtx->streams[m_nVideoIndex]->codecpar);
		else
		{
			DxTraceMsg("%s avcodec_alloc_context3 Failed.\n", __FUNCTION__);
			return false;
		}
		m_pAVCodec = avcodec_find_decoder(m_pAVCtx->codec_id);
		if (!m_pAVCodec)
		{
			DxTraceMsg("%s avcodec_find_decoder failed.\n", __FUNCTION__);
			return false;
		}
		if (bEnableHaccel)
		{
			HRESULT hr = InitD3D(nAdapter);
			if (FAILED(hr))
			{
				DxTraceMsg("-> D3D Initialization failed with hr: %X\n", hr);
				return false;
			}
			if (CodecIsSupported(m_pAVCtx->codec_id))
				AdditionaDecoderInit();
			else
			{
				FreeD3DResources();
				bEnableHaccel = false;
				if (m_pAVCtx->codec_id == AV_CODEC_ID_HEVC)
					m_nManufacturer = HISILICON;
			}
		}

		if (m_nManufacturer == FFMPEG)
			InitFFmpegDecoder(bEnableHaccel);
#ifdef __HisiLicon
		else
			InitHisiliconDecoder();
#else
		else
			return false;
#endif
		m_pFrame = av_frame_alloc();
		m_bInInit = true;
		return true;
	}
	/// @brief 销毁解码器
	STDMETHODIMP DestroyDecoder()
	{
		if (m_nManufacturer == FFMPEG)
			DestroyFFmpegDecoder();
#ifdef __HisiLicon
		else if (m_nManufacturer == HISILICON)
			DestroyHisiliconDecoder();
#endif
		return S_OK;
	}
	/// @brief 根据编码格式，取得指定粒度的字节对齐数
	static DWORD GetAlignedDimension(AVCodecID nCodecID, DWORD dim)
	{
		int align = DXVA2_SURFACE_BASE_ALIGN;

		// MPEG-2 needs higher alignment on Intel cards, and it doesn't seem to harm anything to do it for all cards.
		if (nCodecID == AV_CODEC_ID_MPEG2VIDEO)
			align <<= 1;
		else if (nCodecID == AV_CODEC_ID_HEVC)
			align = 32;

		return FFALIGN(dim, align);
	}

public:
	/// @为硬解码初始化D3D对象
	HRESULT InitD3D(UINT &nAdapter /*= D3DADAPTER_DEFAULT*/);

	/// @为硬解码初始化D3D对象
	HRESULT InitDxva(IDirect3D9Ex  *pD3D, IDirect3DDevice9Ex *pD3DDev);

	/// @brief 把DXVA硬解码过程同FFMpeg解码库关联
	HRESULT AdditionaDecoderInit();

	/// @brief 测试指定的编码格式是支持硬解码（在本机）
	bool CodecIsSupported(AVCodecID nCodec)
	{
		if (!IsWindowsVistaOrGreater())
			return false;
		GUID input = GUID_NULL;
		D3DFORMAT output = D3DFMT_UNKNOWN;		
		if (m_pDXVADecoderService)
		{
			HRESULT hr = FindVideoServiceConversion(nCodec, false, &input, &output);
			if (FAILED(hr))
			{
				DxTraceMsg("No decoder device available that can decode codec '%S' to a matching output.\n", avcodec_get_name(nCodec));
				return false;
			}
			else
				return true;
		}
		else
			return false;
	}
	STDMETHODIMP DestroyDXVADecoder(bool bFull, bool bNoAVCodec = false);
	STDMETHODIMP FreeD3DResources();
	STDMETHODIMP LoadDXVA2Functions();
	HRESULT CreateD3DDeviceManager(IDirect3DDevice9Ex *pDevice, UINT *pReset, IDirect3DDeviceManager9 **ppManager);
	HRESULT CreateDXVAVideoService(IDirect3DDeviceManager9 *pManager, IDirectXVideoDecoderService **ppService);
	HRESULT FindVideoServiceConversion(AVCodecID codec, bool bHighBitdepth, GUID *input, D3DFORMAT *output);
	HRESULT FindDecoderConfiguration(const GUID &input, const DXVA2_VideoDesc *pDesc, DXVA2_ConfigPictureDecode *pConfig);
	HRESULT CreateDXVA2Decoder(int nSurfaces = 0, IDirect3DSurface9 **ppSurfaces = nullptr);
	HRESULT SetD3DDeviceManager(IDirect3DDeviceManager9 *pDevManager);
	HRESULT RetrieveVendorId(IDirect3DDeviceManager9 *pDevManager);
	HRESULT CheckHWCompatConditions(GUID decoderGuid);
	HRESULT FillHWContext(dxva_context *ctx);
	HRESULT ReInitDXVA2Decoder(AVCodecContext *c);
	static enum AVPixelFormat get_dxva2_format(struct AVCodecContext *s, const enum AVPixelFormat * pix_fmts);
	static int get_dxva2_buffer(struct AVCodecContext *c, AVFrame *pic, int flags);
	static void free_dxva2_buffer(void *opaque, uint8_t *data);
	inline IDirect3DDevice9 *GetD3DDevice()
	{
		return m_pD3DDev;
	}

	inline IDirect3D9 *GetD3D9()
	{
		return m_pD3D;
	}

	void FlushDecodeBuffer()
	{
		CAutoLock lock(&m_csDecoder);
		if (m_nManufacturer == FFMPEG)
			avcodec_flush_buffers(m_pAVCtx);
		
	}
	/// @brief 解码
	/// 以下注释摘自于ffmpeg的解码函数avcodec_decode_video2，并有所删减
	/**
	* Decode the video frame of size avpkt->size from avpkt->data into picture.
	* Some decoders may support multiple frames in a single AVPacket, such
	* decoders would then just decode the first frame.
	*
	* @warning The input buffer must be AV_INPUT_BUFFER_PADDING_SIZE larger than
	* the actual read bytes because some optimized bitstream readers read 32 or 64
	* bits at once and could read over the end.
	*
	* @warning The end of the input buffer buf should be set to 0 to ensure that
	* no overreading happens for damaged MPEG streams.
	*
	* @note Codecs which have the AV_CODEC_CAP_DELAY capability set have a delay
	* between input and output, these need to be fed with avpkt->data=NULL,
	* avpkt->size=0 at the end to return the remaining frames.
	* @param[in] avpkt The input AVPacket containing the input buffer.
	*            You can create such packet with av_init_packet() and by then setting
	*            data and size, some decoders might in addition need other fields like
	*            flags&AV_PKT_FLAG_KEY. All decoders are designed to use the least
	*            fields possible.
	* @param[in,out] got_picture_ptr Zero if no frame could be decompressed, otherwise, it is nonzero.
	* @return On error a negative value is returned, otherwise the number of bytes
	* used or zero if no frame could be decompressed.
	*/
	inline int Decode(INOUT AVFrame *pAvFrame, OUT int &got_picture, IN AVPacket *pPacket)
	{
// 		int nSize = pPacket->size - 24;
// 		if (pPacket->data[0] == 0 &&
// 			pPacket->data[1] == 0 &&
// 			pPacket->data[2] == 1 &&
// 			pPacket->data[3] == 0xed
// 			)
// 		{
// 			pPacket->data += 24;
// 			pPacket->size -= 24;
// 		}
// 		if (pPacket->data[nSize] == 0 && 
// 			pPacket->data[nSize+1] == 0 &&
// 			pPacket->data[nSize+2] == 1 &&
// 			pPacket->data[nSize + 3] == 0xec)
// 		{
// 			pPacket->size = nSize;
// 		}
			
		CAutoLock lock(&m_csDecoder);
		if (m_nManufacturer == FFMPEG)
		{
			int nAvError = avcodec_send_packet(m_pAVCtx, pPacket);
			if (nAvError >= 0)
			{
				nAvError = avcodec_receive_frame(m_pAVCtx, pAvFrame);
				if (nAvError == AVERROR(EAGAIN) || nAvError == AVERROR_EOF)
					return nAvError;
				else if (nAvError < 0)
					got_picture = 0;
				else
					got_picture = 1;
			}
			return nAvError;
			//return avcodec_decode_video2(m_pAVCtx, pAvFrame, &got_picture, pPacket);
		}
#ifdef __HisiLicon
		else
		{
			IH265DEC_INARGS stInArgs;
			stInArgs.pStream = pPacket->data;
			stInArgs.uiStreamLen = pPacket->size;
			stInArgs.uiTimeStamp = time(NULL);
			IH265DEC_OUTARGS stOutArgs;
			got_picture = 0;
			char szAvError[1024] = { 0 };
			int nAvError = IHW265D_DecodeAU(m_hDecoder265, &stInArgs, &stOutArgs);
			if (nAvError != IHW265D_OK)
			{
				HisiconError(nAvError, szAvError, 1024);
				DxTraceMsg("IHW265D_DecodeAU Decodec Error:%s.\n", szAvError);
				return -1;
			}

			if (stOutArgs.eDecodeStatus == IH265D_NO_PICTURE)
			{
				return 0;
			}
			// output decoded pictures
			if (stOutArgs.eDecodeStatus == IH265D_GETDISPLAY)
			{
				HisiconH265Frame2FFmpegAvFrame(&stOutArgs, pAvFrame);
				got_picture = 1;
				return pPacket->size;
			}
			return 0;
		}
#else
		return -1;
#endif
	}
	/// @brief 移动到指定帧，只支持ffmpeg文件解码
	inline int SeekFrame(int64_t timestamp, int flags)
	{
		if (!m_pFormatCtx)
			return -1;
		return av_seek_frame(m_pFormatCtx, m_nVideoIndex, timestamp, flags);
	}
	/// @brief 读取一帧，只支持ffmpeg文件解码
	inline int ReadFrame(AVPacket *pkt)
	{
		if (!m_pFormatCtx)
			return -1;
		return av_read_frame(m_pFormatCtx, pkt);
	}

public:
	long					m_nVtableAddr;		// 虚函数表地址，该变量地址位置虚函数表之后，仅用于类初始化，请匆移动该变量的位置
	struct
	{
		HMODULE dxva2lib;
		pCreateDeviceManager9 *createDeviceManager;
	} m_dxva;
	HMODULE					m_hD3D9 = nullptr;
	IDirect3D9Ex            *m_pD3D = nullptr;
	IDirect3DDevice9Ex      *m_pD3DDev = nullptr;
	bool					m_bD3DShared = false;
	pD3DCreate9Ex			*m_pDirect3DCreate9Ex;
	IDirect3DDeviceManager9 *m_pD3DDevMngr = nullptr;
	UINT                    m_pD3DResetToken = 0;
	HANDLE                  m_hDevice = INVALID_HANDLE_VALUE;
	IDirectXVideoDecoderService *m_pDXVADecoderService = nullptr;
	IDirectXVideoDecoder        *m_pDecoder = nullptr;
	DXVA2_ConfigPictureDecode   m_DXVAVideoDecoderConfig;
	int					m_NumSurfaces = 0;
	d3d_surface_t		m_pSurfaces[DXVA2_MAX_SURFACES];
	uint64_t			m_CurrentSurfaceAge = 1;
	LPDIRECT3DSURFACE9	m_pRawSurface[DXVA2_MAX_SURFACES];
	AVPixelFormat		m_DecoderPixelFormat = AV_PIX_FMT_NONE;
	DWORD				m_dwSurfaceWidth = 0;
	DWORD				m_dwSurfaceHeight = 0;
	D3DFORMAT			m_eSurfaceFormat = D3DFMT_UNKNOWN;
	DWORD				m_dwVendorId = 0;
	DWORD				m_dwDeviceId = 0;
	GUID				m_guidDecoderDevice = GUID_NULL;
	int					m_DisplayDelay = DXVA2_QUEUE_SURFACES;
	AVCodecContext      *m_pAVCtx = nullptr;
	int					m_nDecodeThreadCount = 0;
	AVFormatContext		*m_pFormatCtx = nullptr;
	AVIOContext			*m_pIoContext = nullptr;
	AvQueue				*m_pAvQueue = nullptr;
	bool				m_bProbeSucceed = false;
	AVFrame             *m_pFrame = nullptr;
	AVCodecID           m_nCodecId = AV_CODEC_ID_NONE;
	BOOL                m_bInInit = FALSE;
	AVCodec             *m_pAVCodec = nullptr;
	SwsContext          *m_pSwsContext = nullptr;	
	int					m_nVideoIndex = -1;
	int					m_nAudioIndex = -1;
	D3DFORMAT			m_nD3DFormat;
	UINT				m_nWidth;
	UINT				m_nHeight;
	HWND				m_hPresentWnd;
	LPDIRECT3DSURFACE9	m_pDirect3DSurfaceRender;
	D3DPRESENT_PARAMETERS m_d3dpp;
	static	CAvRegister AvRegister;
	// 实时流播放相关变量
	uint8_t				*m_pAvBuffer = nullptr;
	int					m_nAvBufferSize = 512*1024;
	
	AVInputFormat *m_pInputFormatCtx = nullptr;
private:
	Decoder_Manufacturer m_nManufacturer = FFMPEG;
	// 海思265解码器相关变量
	IH265DEC_HANDLE m_hDecoder265;
	IHW265D_INIT_PARAM m_stInitParam;
	IH265DEC_INARGS m_stInArgs;
	IH265DEC_OUTARGS m_stOutArgs;
	CRITICAL_SECTION  m_csDecoder;
};

CVideoDecoder *CreateDecoderDXVA2();
typedef std::tr1::shared_ptr<CVideoDecoder> CVideoDecoderPtr;