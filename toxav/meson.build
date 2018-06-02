raspberrypi = false

libtoxav_files = files(
	'../toxav/rtp.c',
	'../toxav/msi.h',
	'../toxav/msi.c',
	'../toxav/groupav.h',
	'../toxav/groupav.c',
	'../toxav/audio.h',
	'../toxav/audio.c',
	'../toxav/video.h',
	'../toxav/video.c',
	'../toxav/bwcontroller.h',
	'../toxav/bwcontroller.c',
	'../toxav/ring_buffer.h',
	'../toxav/ring_buffer.c',
	'../toxav/toxav.h',
	'../toxav/toxav.c',
	'../toxav/toxav_old.c',
	'../toxav/vpx.c',
	'../toxav/h264.c',
)

defines = []
omx_dep = []

if (raspberrypi_3)
    libtoxav_files += files('../toxav/h264_omx.c')
    defines += '-DRASPBERRY_PI'

    omx_dep = declare_dependency(
        include_directories: include_directories('/opt/vc/include', 'ilclient')

    )
endif


libtoxav = static_library(
	'toxav',
	libtoxav_files,
    dependencies: [opus, vpx, avcodec, avutil, x264, omx_dep],
    c_args: defines
)


