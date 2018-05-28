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
)


libtoxav = static_library(
	'toxav',
	libtoxav_files,
    dependencies: [opus, vpx, avcodec, avutil, x264]
  
  #include_directories : inc_common,
  #cpp_args: internal_args,
)


