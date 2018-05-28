project('toxcore', 'c', license: 'GPL')

compiler = meson.get_compiler('c')

raspberrypi_3 = false

if (raspberrypi_3)
add_global_arguments('-O3', '-g', '-marm', '-march=armv8-a+crc', '-mtune=cortex-a53', '-mfpu=neon-fp-armv8',
	'-mfloat-abi=hard', '-ftree-vectorize', '-funsafe-math-optimizations',
	language: ['c', 'c++'])
endif

sodium = dependency('libsodium')

# ToxAV dependencies
opus = dependency('opus')
vpx = dependency('vpx')
avcodec = dependency('libavcodec')
avutil = dependency('libavutil')
x264 = dependency('x264')

# av_test dependencies
opencv = dependency('opencv')
portaudio = dependency('portaudio-2.0')

subdir('toxcore')
subdir('toxav')

#toxcore_files

#toxcore_lib('toxcore',
#	toxcore_files,
#	c_args: special_args,
#	dependencies: deps)

#toxcore_dep = declare_dependency(
#	
#	)

executable('av_test', 'testing/av_test.c',
	link_with: [libtoxcore, libtoxav],
	dependencies: [opencv, portaudio],
	c_args: ['-x', 'c++']
)

executable('DHT_test', 'testing/DHT_test.c',
	link_with: libtoxcore)

executable('Messenger_test', 'testing/Messenger_test.c',
	link_with: libtoxcore)

