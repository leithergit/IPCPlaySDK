#ifndef _IPC_MEDIA_2015_09_07
#define _IPC_MEDIA_2015_09_07
#include <Windows.h>
#include <WTypes.h>
#include <time.h>
/// IPC录像文件格式
#define IPC_TAG                 0x44564F4D //"MOVD"
#define GSJ_TAG					0x48574D49 //"IMWH"
#define IPC_IPC_SDK_GSJ_HEADER			   0xfde8
#define IPC_IPC_SDK_VERSION_2015_09_07     0x62
#define IPC_IPC_SDK_VERSION_2015_10_20     0x63		///< 版本升级,2015-10-20 因为IPC_MEDIAINFO增加nFps字段,该字段之占用了之前的nCameraType
													///< 字段的位置,为区别这种差别,以版本号区分之,当版本号为0x62时,若遇到fps为0或1，则强制
													///< nFps=25,否则使用IPC_MEDIAINFO中的nFps字段
#define IPC_IPC_SDK_VERSION_2015_12_16     0x64		///< 版本升级,2015-11-09 因为IPCFrameHeader中没有帧序号,在播放时无法知道文件总帧数,必
													///< 须对文件进行遍历,才能获得总帧数,升级后的IPCFrameHeader,增加nFrameTime字段,用于标识
													///< 视频和音频帐ID,播放时,只需要取得最一帧的帧ID,便可知道总帧数,提高了效率。
#define IPC_IPC_SDK_VERSION     IPC_IPC_SDK_VERSION_2015_12_16		

/// @brief 帧类型
enum FrameType
{
	FRAME_UNKNOWN	 = -1,
	FRAME_IDR		 = 1,
	FRAME_I			 = 2,
	FRAME_P			 = 3,
	FRAME_B			 = 4,
	FRAME_JPEG		 = 5,
	FRAME_G711A		 = 6,
	FRAME_G711U		 = 7,
	FRAME_G726		 = 8,
	FRAME_AAC		 = 9,
	Frame_DATA		 = 10
};

/// @brief 编码类型
enum IPC_CODEC
{
	CODEC_UNKNOWN = -1,
	CODEC_H264		 = 0x00,
	CODEC_MJPEG		 = 0x01,
	CODEC_H265		 = 0x02,
	CODEC_G711A		 = FRAME_G711A,
	CODEC_G711U		 = FRAME_G711U,
	CODEC_G726		 = FRAME_G726,
	CODEC_AAC		 = FRAME_AAC
};

#pragma pack(push)
#pragma pack(1)
///@brief IPC私有录像文件头
struct 	IPC_MEDIAINFO
{
	IPC_MEDIAINFO()
	{
		ZeroMemory(this, sizeof(IPC_MEDIAINFO));
		nMediaTag	 = IPC_TAG;
		nSDKversion	 = IPC_IPC_SDK_VERSION;
	}
	~IPC_MEDIAINFO()
	{
	}
	unsigned int 	nMediaTag;		///< 头标志 固定为   0x44564F4D 即字符串"MOVD",对于老版的录像文件亦有可能为GSJ_TAG，即“IMWH”
	long			nSDKversion;	///< 固定值为	IPC_IPC_SDK_VERSION
									///< 版本升级,2015-12-26
									///< 因为IPC_MEDIAINFO增加nFps字段,该字段之占用了之前的nCameraType
									///< 字段的位置,为区别这种差别,以版本号区分之,当版本号为0x62时,若遇到fps为0或1，则强制
									///< nFps=25,否则使用IPC_MEDIAINFO中的nFps字段
	IPC_CODEC		nVideoCodec;	///< 视频编码类型
	IPC_CODEC		nAudioCodec;	///< 音频编码类型
	int				nVideoWidth;	///< 视频图像宽度，可设置为0
	int				nVideoHeight;	///< 视频图像高度，可设置为0
	unsigned char	nFps;			///< 帧率
	unsigned char	nCameraType;	///< 相机类型IPC相机为0，安讯士相机为1 fps
	unsigned char	reserved1[2];	///< 保留,必须置0
	unsigned int	reserved2[3];	///< 保留,必须置0
};

/// @brief IPC私有录像帧头
struct IPCFrameHeader
{
	IPCFrameHeader()
	{
		ZeroMemory(this,sizeof(IPCFrameHeader));
		nFrameTag		 = IPC_TAG;
		nFrameUTCTime	 = (long)time(NULL);
	}
	~IPCFrameHeader()
	{
	}
	long	nLength;				///< 码流数据长度,以字节为单位
	long	nType;					///< 取值于枚举类型FrameType
	__int64	nTimestamp;				///< 时间戳,单位微秒
	long	nFrameTag;				///< IPC_TAG
	long	nFrameUTCTime;			///< 收到帧的utc时间
};
/// @brief IPC私有录像扩展帧头
struct IPCFrameHeaderEx:public IPCFrameHeader
{
	IPCFrameHeaderEx()
	{
		ZeroMemory(this,sizeof(IPCFrameHeaderEx));
		nFrameTag		 = IPC_TAG;
		nFrameUTCTime	 = (long)time(NULL);
	}
	unsigned int	nFrameID;		///< 帧序号,视频帧和音频帧分别独立计数,从0开始计数,即第一帧nFrameID为0
#ifndef _DEBUG
	unsigned int	nReserver[3];	///< 保留字段,建议置0
#else
	unsigned int	nReserver;		///< 保留字段,建议置0
	double          dfRecvTime;
#endif
};
#pragma pack(pop)

/*
每个录像文件的开始位置，写入固定大小为40个字节的文件头,内容为IPC_MEDIAINFO结构体内容。
每一帧数据，前面加上一个固定大小为24个字节的帧头，即每一帧的属性信息,内容为stream_head结构体内容，紧接着写入帧数据。

格式举例：
文件头
帧属性+帧数据
帧属性+帧数据
...
帧属性+帧数据
*/

#endif	//_IPC_MEDIA_2015_09_07