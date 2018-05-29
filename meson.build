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

#toxcore_files

#toxcore_lib('toxcore',
#	toxcore_files,
#	c_args: special_args,
#	dependencies: deps)


# A workaround to represent the final include directory structure
#configure_file(input: 'toxcore/tox.h', install_dir: 'tox', output: 'tox.h',
#	configuration: configuration_data())

toxcore = declare_dependency(
	link_with: libtoxcore
	)

toxav = declare_dependency(
	link_with: libtoxav
	)

executable('av_test', 'testing/av_test.c',
	dependencies: [toxcore, toxav, opencv, portaudio],
	c_args: ['-x', 'c++']
)

executable('DHT_test', 'testing/DHT_test.c',
	link_with: libtoxcore)

executable('Messenger_test', 'testing/Messenger_test.c',
	link_with: libtoxcore)

