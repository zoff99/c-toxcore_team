# This hack places the header files in a appropriate directory that represents the path it is finally installed in

headers = []

conf = configuration_data()

headers += configure_file(input: '../toxcore/tox.h', output: 'tox.h', configuration: conf)
headers += configure_file(input: '../toxutil/toxutil.h', output: 'toxutil.h', configuration: conf)
headers += configure_file(input: '../toxav/toxav.h', output: 'toxav.h', configuration: conf)

