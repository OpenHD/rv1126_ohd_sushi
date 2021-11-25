版本信息：
- 支持多sensor

commit 信息：
	- rkaiq_tool_server-socket: 8c98684 The support of socket version for double shot is added
	- rkaiq_tool_server: d61bc6758  sync code: rkaiq_tool_server no-socket version.

rkaiq_tool_server
该应用为Tuning时使用的相机应用，负责打开相机、取流、预览、在校调试通信等，会与用户应用相冲突，连接工具时工具会将该应用推送至板端运行，连接之前应确保板端的相机应用关闭，否则会连不上工具。
该应用运行时会启动RTSP服务，用户可以通过VLC等软件查看网络串流的实时预览。

rkaiq_tool_server-socket
该应用仅提供在线调试通信功能，可以与用户应用共存，但目前尚未支持拍摄Raw、拍摄YUV功能。
预览显示则依赖于用户的相机应用提供的预览方式。
