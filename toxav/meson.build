libtoxav_files = files(
	'rtp.c',
	'msi.h',
	'msi.c',
	'groupav.h',
	'groupav.c',
	'audio.h',
	'audio.c',
	'video.h',
	'video.c',
	'bwcontroller.h',
	'bwcontroller.c',
	'ring_buffer.h',
	'ring_buffer.c',
	'toxav.h',
	'toxav.c',
	'toxav_old.c',
	'codecs/vpx/codec.c',
	'codecs/h264/codec.c',
)

omx_dep = []

if (raspberrypi)
    libtoxav_files += files('codecs/h264_omx_raspi/codec.c')
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


