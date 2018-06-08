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

omx_dep = []

if (raspberrypi)
    libtoxav_files += files('../toxav/h264_omx.c')
    bcm_defines = ['-DRASPBERRY_PI', '-DOMX_SKIP64BIT', '-DUSE_VCHIQ_ARM']

	bcm_prefix = '/opt/vc/'
	bcm_incdirs = include_directories(
		bcm_prefix + 'include',
		bcm_prefix + 'include/interface/vmcs_host/linux',
		bcm_prefix + 'include/interface/vcos/pthreads',
		)

    omx_dep = declare_dependency(
        include_directories: bcm_incdirs,
		compile_args: bcm_defines,
		link_args: ['-L/opt/vc/lib', '-lbcm_host', '-lvcos','-lopenmaxil'],

    )
endif


libtoxav = static_library(
	'toxav',
	libtoxav_files,
    dependencies: [opus, vpx, avcodec, avutil, x264, omx_dep],
)


