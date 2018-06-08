project('toxcore', 'c', license: 'GPL')

compiler = meson.get_compiler('c')

raspberrypi = true

#if (raspberrypi_3)
#add_global_arguments('-O3', '-g', '-marm', '-march=armv8-a+crc', '-mtune=cortex-a53', '-mfpu=neon-fp-armv8',
#	'-mfloat-abi=hard', '-ftree-vectorize', '-funsafe-math-optimizations',
#	language: ['c', 'c++'])
#endif

sodium = dependency('libsodium')

# ToxAV dependencies
threads = dependency('threads')
opus = dependency('opus')
vpx = dependency('vpx')
avcodec = dependency('libavcodec')
avutil = dependency('libavutil')
x264 = dependency('x264')

# av_test dependencies
opencv = dependency('opencv', required: false)
portaudio = dependency('portaudio-2.0', required: false)

subdir('toxcore')
subdir('toxav')

subdir('tox')

incdir = include_directories('.')

toxcore = declare_dependency(
	link_with: libtoxcore,
	include_directories: incdir,
	sources: headers
	)

toxav = declare_dependency(
	link_with: libtoxav,
	include_directories: incdir
	)

if opencv.found() and portaudio.found() and sndfile.found()
	executable('av_test', 'testing/av_test.c',
		dependencies: [toxcore, toxav, opencv, portaudio, threads, omx_dep],
		c_args: ['-x', 'c++']
	)
else
	message('Not building av_test due to lack of dependencies')
endif

executable('DHT_test', 'testing/DHT_test.c',
	link_with: libtoxcore)

executable('Messenger_test', 'testing/Messenger_test.c',
	link_with: libtoxcore)

