

## v0.2.3

### Merged PRs:

- [#951](https://github.com/TokTok/c-toxcore/pull/951) Only run astyle if the astyle binary exists.
- [#950](https://github.com/TokTok/c-toxcore/pull/950) Remove utils.c and utils.h from toxencryptsave build.
- [#949](https://github.com/TokTok/c-toxcore/pull/949) Fixes to the imported sodium sources to compile without warnings.
- [#948](https://github.com/TokTok/c-toxcore/pull/948) Add a MAX_HOSTNAME_LENGTH constant.
- [#947](https://github.com/TokTok/c-toxcore/pull/947) Remove the format test.
- [#937](https://github.com/TokTok/c-toxcore/pull/937) Add new Circle CI configuration.
- [#935](https://github.com/TokTok/c-toxcore/pull/935) Add a test for double conference invite.
- [#933](https://github.com/TokTok/c-toxcore/pull/933) Add Logger to various net_crypto functions, and add `const` to Logger where possible.
- [#931](https://github.com/TokTok/c-toxcore/pull/931) Avoid conditional-uninitialised warning for tcp test.
- [#930](https://github.com/TokTok/c-toxcore/pull/930) Disable UDP when proxy is enabled.
- [#928](https://github.com/TokTok/c-toxcore/pull/928) Use clang-format for C++ code.
- [#927](https://github.com/TokTok/c-toxcore/pull/927) Add assertions to bootstrap tests for correct connection type.
- [#926](https://github.com/TokTok/c-toxcore/pull/926) Make NULL options behave the same as default options.
- [#925](https://github.com/TokTok/c-toxcore/pull/925) Add tests for what happens when passing an invalid proxy host.
- [#924](https://github.com/TokTok/c-toxcore/pull/924) Make the net_crypto connection state an enum.
- [#922](https://github.com/TokTok/c-toxcore/pull/922) Clarify/Improve test_some test
- [#921](https://github.com/TokTok/c-toxcore/pull/921) Beginnings of a TCP_test.c overhaul
- [#920](https://github.com/TokTok/c-toxcore/pull/920) Add test for creating multiple conferences in one tox.
- [#918](https://github.com/TokTok/c-toxcore/pull/918) Merge irungentoo/master into toktok
- [#917](https://github.com/TokTok/c-toxcore/pull/917) Add random testing program.
- [#916](https://github.com/TokTok/c-toxcore/pull/916) Fix linking with address sanitizer.
- [#915](https://github.com/TokTok/c-toxcore/pull/915) Remove resource_leak_test.
- [#914](https://github.com/TokTok/c-toxcore/pull/914) Make dht_test more stable.
- [#913](https://github.com/TokTok/c-toxcore/pull/913) Minor cleanup: return early on error condition.
- [#906](https://github.com/TokTok/c-toxcore/pull/906) Sort bazel build file according to buildifier standard.
- [#905](https://github.com/TokTok/c-toxcore/pull/905) In DEBUG mode, make toxcore crash on signed integer overflow.
- [#902](https://github.com/TokTok/c-toxcore/pull/902) Log only the filename, not the full path in LOGGER.
- [#899](https://github.com/TokTok/c-toxcore/pull/899) Fix macOS macro because of GNU Mach
- [#898](https://github.com/TokTok/c-toxcore/pull/898) Fix enumeration of Crypto_Connection instances
- [#897](https://github.com/TokTok/c-toxcore/pull/897) Fix ipport_isset: port 0 is not a valid port.
- [#894](https://github.com/TokTok/c-toxcore/pull/894) Fix logging related crash in bootstrap node
- [#893](https://github.com/TokTok/c-toxcore/pull/893) Fix bootstrap crashes, still
- [#892](https://github.com/TokTok/c-toxcore/pull/892) Add empty logger to DHT bootstrap daemons.
- [#887](https://github.com/TokTok/c-toxcore/pull/887) Fix FreeBSD build on Travis
- [#884](https://github.com/TokTok/c-toxcore/pull/884) Fix the often call of event tox_friend_connection_status
- [#883](https://github.com/TokTok/c-toxcore/pull/883) Make toxcore compile on BSD
- [#878](https://github.com/TokTok/c-toxcore/pull/878) fix DHT_bootstrap key loading
- [#877](https://github.com/TokTok/c-toxcore/pull/877) Add minitox to under "Other resources" section in the README
- [#875](https://github.com/TokTok/c-toxcore/pull/875) Make bootstrap daemon use toxcore's version
- [#867](https://github.com/TokTok/c-toxcore/pull/867) Improve network error reporting on Windows
- [#841](https://github.com/TokTok/c-toxcore/pull/841) Only check full rtp offset if RTP_LARGE_FRAME is set
- [#823](https://github.com/TokTok/c-toxcore/pull/823) Finish @Diadlo's network Family abstraction.
- [#822](https://github.com/TokTok/c-toxcore/pull/822) Move system header includes from network.h to network.c

## v0.2.2

### Merged PRs:

- [#872](https://github.com/TokTok/c-toxcore/pull/872) Restrict packet kinds that can be sent through onion path.
- [#864](https://github.com/TokTok/c-toxcore/pull/864) CMake warn if libconfig not found
- [#863](https://github.com/TokTok/c-toxcore/pull/863) Remove broken and unmaintained scripts.
- [#862](https://github.com/TokTok/c-toxcore/pull/862) Release v0.2.2
- [#859](https://github.com/TokTok/c-toxcore/pull/859) Add clarifying comment to cryptpacket_received function.
- [#857](https://github.com/TokTok/c-toxcore/pull/857) Avoid the use of rand() in tests.
- [#852](https://github.com/TokTok/c-toxcore/pull/852) bugfix build error on MacOS
- [#846](https://github.com/TokTok/c-toxcore/pull/846) Disallow stderr logger by default.
- [#845](https://github.com/TokTok/c-toxcore/pull/845) Fix coveralls reporting.
- [#844](https://github.com/TokTok/c-toxcore/pull/844) Add COVERAGE cmake flag for clang.
- [#825](https://github.com/TokTok/c-toxcore/pull/825) Add default stderr logger for logging to nullptr.
- [#824](https://github.com/TokTok/c-toxcore/pull/824) Simplify sendpacket function, deduplicate some logic.
- [#809](https://github.com/TokTok/c-toxcore/pull/809) Remove the use of the 'hh' format specifier.
- [#801](https://github.com/TokTok/c-toxcore/pull/801) Add logging to the onion_test.
- [#797](https://github.com/TokTok/c-toxcore/pull/797) Move struct DHT_Friend into DHT.c.

## v0.2.1

### Merged PRs:

- [#839](https://github.com/TokTok/c-toxcore/pull/839) Update changelog for 0.2.1
- [#837](https://github.com/TokTok/c-toxcore/pull/837) Update version to 0.2.1.
- [#833](https://github.com/TokTok/c-toxcore/pull/833) Add missing tox_nospam_size() function
- [#832](https://github.com/TokTok/c-toxcore/pull/832) Don't set RTP_LARGE_FRAME on rtp audio packets
- [#831](https://github.com/TokTok/c-toxcore/pull/831) Don't throw away rtp packets from old Toxcore
- [#828](https://github.com/TokTok/c-toxcore/pull/828) Make file transfers 50% faster.

## v0.2.0

### Merged PRs:

- [#821](https://github.com/TokTok/c-toxcore/pull/821) Remove deprecated conference namelist change callback.
- [#820](https://github.com/TokTok/c-toxcore/pull/820) Fix auto_tests to stop using the deprecated conference API.
- [#819](https://github.com/TokTok/c-toxcore/pull/819) Change default username to empty string
- [#818](https://github.com/TokTok/c-toxcore/pull/818) Change README to talk about cmake instead of autoreconf.
- [#817](https://github.com/TokTok/c-toxcore/pull/817) Fix warning on Mac OS X and FreeBSD.
- [#815](https://github.com/TokTok/c-toxcore/pull/815) Some minor cleanups suggested by cppcheck.
- [#814](https://github.com/TokTok/c-toxcore/pull/814) Fix memory leak of Logger instance on error paths.
- [#813](https://github.com/TokTok/c-toxcore/pull/813) Minor cleanups: dead stores and avoiding complex macros.
- [#811](https://github.com/TokTok/c-toxcore/pull/811) Update changelog for 0.2.0
- [#808](https://github.com/TokTok/c-toxcore/pull/808) Fix a bunch of compiler warnings and remove suppressions.
- [#807](https://github.com/TokTok/c-toxcore/pull/807) Link all tests to the android cpufeatures library if available.
- [#806](https://github.com/TokTok/c-toxcore/pull/806) Fix toxcore.pc generation.
- [#805](https://github.com/TokTok/c-toxcore/pull/805) Add an option that allows us to specify that we require toxav.
- [#804](https://github.com/TokTok/c-toxcore/pull/804) Fix OSX tests: find(1) doesn't work like on Linux.
- [#803](https://github.com/TokTok/c-toxcore/pull/803) Fix the windows build: pthread needs to be linked after vpx.
- [#800](https://github.com/TokTok/c-toxcore/pull/800) Make group number in the toxav public API uint32_t
- [#799](https://github.com/TokTok/c-toxcore/pull/799) Implement the "persistent conference" callback changes as new functions.
- [#798](https://github.com/TokTok/c-toxcore/pull/798) Add deprecation notices to functions that will go away in v0.3.0.
- [#796](https://github.com/TokTok/c-toxcore/pull/796) Make some sizeof tests linux-only.
- [#794](https://github.com/TokTok/c-toxcore/pull/794) Remove apidsl from the build.
- [#793](https://github.com/TokTok/c-toxcore/pull/793) Add a bazel test that ensures all our projects are GPL-3.0.
- [#792](https://github.com/TokTok/c-toxcore/pull/792) Increase range of ports available to Toxes during tests
- [#791](https://github.com/TokTok/c-toxcore/pull/791) Run all tests in parallel on Travis.
- [#790](https://github.com/TokTok/c-toxcore/pull/790) Disable lan discovery in most tests.
- [#789](https://github.com/TokTok/c-toxcore/pull/789) Remove tox_test from autotools build.
- [#788](https://github.com/TokTok/c-toxcore/pull/788) Don't print trace level logging in tests.
- [#787](https://github.com/TokTok/c-toxcore/pull/787) Split up tox_test into multiple smaller tests
- [#783](https://github.com/TokTok/c-toxcore/pull/783) Send 0 as peer number in CHANGE_OCCURRED group event.
- [#782](https://github.com/TokTok/c-toxcore/pull/782) Use `const` more in C code.
- [#781](https://github.com/TokTok/c-toxcore/pull/781) Don't build all the small sub-libraries.
- [#780](https://github.com/TokTok/c-toxcore/pull/780) Get rid of the only GNU extension we used.
- [#779](https://github.com/TokTok/c-toxcore/pull/779) Remove leftover symmetric key from DHT struct.
- [#778](https://github.com/TokTok/c-toxcore/pull/778) Add static asserts for all the struct sizes in toxcore.
- [#776](https://github.com/TokTok/c-toxcore/pull/776) Optionally use newer cmake features.
- [#774](https://github.com/TokTok/c-toxcore/pull/774) Improve gtest finding, support local checkout.
- [#773](https://github.com/TokTok/c-toxcore/pull/773) Add gtest include directory to -I flags if found.
- [#772](https://github.com/TokTok/c-toxcore/pull/772) Reject discovery packets coming from outside the "LAN".
- [#771](https://github.com/TokTok/c-toxcore/pull/771) Adopt the "change occurred" API change from isotoxin-groupchat.
- [#770](https://github.com/TokTok/c-toxcore/pull/770) Add MSVC compilation instructions
- [#767](https://github.com/TokTok/c-toxcore/pull/767) Build toxcore with libsodium.dll instead of libsodium.lib.
- [#766](https://github.com/TokTok/c-toxcore/pull/766) Remove libcheck from the dependencies.
- [#764](https://github.com/TokTok/c-toxcore/pull/764) Fix LAN discovery on FreeBSD.
- [#760](https://github.com/TokTok/c-toxcore/pull/760) Make cmake script more forgiving.
- [#759](https://github.com/TokTok/c-toxcore/pull/759) Use more ubuntu packages; remove hstox for now.
- [#757](https://github.com/TokTok/c-toxcore/pull/757) Improve stability of crypto_memcmp test.
- [#756](https://github.com/TokTok/c-toxcore/pull/756) Format .cpp files with format-source.
- [#755](https://github.com/TokTok/c-toxcore/pull/755) Add some unit tests for util.h.
- [#754](https://github.com/TokTok/c-toxcore/pull/754) Move the tox_sync tool to the toxins repository.
- [#753](https://github.com/TokTok/c-toxcore/pull/753) Move irc_syncbot to the toxins repository.
- [#752](https://github.com/TokTok/c-toxcore/pull/752) Move tox_shell program to the toxins repository.
- [#751](https://github.com/TokTok/c-toxcore/pull/751) Use the markdown GPLv3 license in the c-toxcore repo.
- [#750](https://github.com/TokTok/c-toxcore/pull/750) Remove csrc from the RTPHeader struct.
- [#748](https://github.com/TokTok/c-toxcore/pull/748) Revert "Add correction message type"
- [#745](https://github.com/TokTok/c-toxcore/pull/745) Change the "capabilities" field to a "flags" field.
- [#742](https://github.com/TokTok/c-toxcore/pull/742) Improve conference test stability.
- [#741](https://github.com/TokTok/c-toxcore/pull/741) Add `-D__STDC_LIMIT_MACROS=1` for C++ code.
- [#739](https://github.com/TokTok/c-toxcore/pull/739) Add RTP header fields for the full frame length and offset.
- [#737](https://github.com/TokTok/c-toxcore/pull/737) Use nullptr as NULL pointer constant instead of NULL or 0.
- [#736](https://github.com/TokTok/c-toxcore/pull/736) Avoid clashes with "build" directories on case-insensitive file systems.
- [#734](https://github.com/TokTok/c-toxcore/pull/734) Make audio/video bit rates "properties"
- [#733](https://github.com/TokTok/c-toxcore/pull/733) Fix link in README.md
- [#730](https://github.com/TokTok/c-toxcore/pull/730) Fix out of bounds read in error case in messenger_test.
- [#729](https://github.com/TokTok/c-toxcore/pull/729) Remove dead return statement.
- [#728](https://github.com/TokTok/c-toxcore/pull/728) Disable the autotools build in PR builds.
- [#727](https://github.com/TokTok/c-toxcore/pull/727) Rename some rtp header struct members to be clearer.
- [#725](https://github.com/TokTok/c-toxcore/pull/725) Publish a single public BUILD target for c-toxcore.
- [#723](https://github.com/TokTok/c-toxcore/pull/723) Use <stdlib.h> for alloca on FreeBSD.
- [#722](https://github.com/TokTok/c-toxcore/pull/722) Use self-built portaudio instead of system-provided.
- [#721](https://github.com/TokTok/c-toxcore/pull/721) Manually serialise RTPHeader struct instead of memcpy.
- [#718](https://github.com/TokTok/c-toxcore/pull/718) Improve sending of large video frames in toxav.
- [#716](https://github.com/TokTok/c-toxcore/pull/716) Add comment from #629 in ring_buffer.c.
- [#714](https://github.com/TokTok/c-toxcore/pull/714) Make BUILD files more finely-grained.
- [#713](https://github.com/TokTok/c-toxcore/pull/713) Add BUILD files for all the little tools in the repo.
- [#711](https://github.com/TokTok/c-toxcore/pull/711) Make the monolith test a C++ binary.
- [#710](https://github.com/TokTok/c-toxcore/pull/710) Don't allocate or dereference Tox_Options in tests.
- [#709](https://github.com/TokTok/c-toxcore/pull/709) Remove nTox from the repo.
- [#708](https://github.com/TokTok/c-toxcore/pull/708) Add testing/*.c (except av_test) to bazel build.
- [#707](https://github.com/TokTok/c-toxcore/pull/707) Fix log message in simple_conference_test: invite -> message.
- [#703](https://github.com/TokTok/c-toxcore/pull/703) Add a simple conference test with 3 friends.
- [#701](https://github.com/TokTok/c-toxcore/pull/701) Add astyle to Circle CI build.
- [#700](https://github.com/TokTok/c-toxcore/pull/700) Use more descriptive names in bwcontroller.
- [#699](https://github.com/TokTok/c-toxcore/pull/699) Add some explanatory comments to the toxav audio code.
- [#698](https://github.com/TokTok/c-toxcore/pull/698) Extract named constants from magic numbers in toxav/audio.c.
- [#697](https://github.com/TokTok/c-toxcore/pull/697) Use C99 standard in bazel builds.
- [#694](https://github.com/TokTok/c-toxcore/pull/694) Add bazel build scripts for c-toxcore.
- [#693](https://github.com/TokTok/c-toxcore/pull/693) Make libcheck optional for windows builds.
- [#691](https://github.com/TokTok/c-toxcore/pull/691) Don't install packages needlessly on Travis
- [#690](https://github.com/TokTok/c-toxcore/pull/690) Run fewer Travis jobs during Pull Requests.
- [#689](https://github.com/TokTok/c-toxcore/pull/689) Make Net_Crypto a module-private type.
- [#688](https://github.com/TokTok/c-toxcore/pull/688) Make DHT a module-private type.
- [#687](https://github.com/TokTok/c-toxcore/pull/687) Use apidsl to generate LAN_discovery.h.
- [#686](https://github.com/TokTok/c-toxcore/pull/686) Remove hstox test for now.
- [#685](https://github.com/TokTok/c-toxcore/pull/685) Add message type for correction
- [#684](https://github.com/TokTok/c-toxcore/pull/684) Add random_u16 function and rename the others to match.
- [#682](https://github.com/TokTok/c-toxcore/pull/682) Use larger arrays in crypto timing tests.
- [#681](https://github.com/TokTok/c-toxcore/pull/681) Fix some memory or file descriptor leaks in test code.
- [#680](https://github.com/TokTok/c-toxcore/pull/680) Filter out annoying log statements in unit tests.
- [#679](https://github.com/TokTok/c-toxcore/pull/679) Use apidsl to generate ping.h.
- [#678](https://github.com/TokTok/c-toxcore/pull/678) Sort monolith.h according to ls(1): uppercase first.
- [#677](https://github.com/TokTok/c-toxcore/pull/677) Make pack/unpack_ip_port public DHT functions.
- [#675](https://github.com/TokTok/c-toxcore/pull/675) Make Onion_Announce a module-private type.
- [#674](https://github.com/TokTok/c-toxcore/pull/674) Make TCP_Client_Connection a module-private type.
- [#673](https://github.com/TokTok/c-toxcore/pull/673) Move TCP_Secure_Connection from .h to .c file.
- [#672](https://github.com/TokTok/c-toxcore/pull/672) Make Friend_Connections a module-private type.
- [#670](https://github.com/TokTok/c-toxcore/pull/670) Make Friend_Requests a module-private type.
- [#669](https://github.com/TokTok/c-toxcore/pull/669) Make Onion_Client a module-private type.
- [#668](https://github.com/TokTok/c-toxcore/pull/668) Make Ping_Array a module-private type.
- [#667](https://github.com/TokTok/c-toxcore/pull/667) pkg-config .pc files: added .private versions of Libs and Required
- [#665](https://github.com/TokTok/c-toxcore/pull/665) Remove useless if statement
- [#662](https://github.com/TokTok/c-toxcore/pull/662) Move Networking_Core struct into the .c file.
- [#661](https://github.com/TokTok/c-toxcore/pull/661) Disable asan, since it seems to break on travis.
- [#660](https://github.com/TokTok/c-toxcore/pull/660) Increase test retries to 10 (basically infinite).
- [#659](https://github.com/TokTok/c-toxcore/pull/659) Fix formatting in some C files.
- [#658](https://github.com/TokTok/c-toxcore/pull/658) Call freeaddrinfo on error paths in net_getipport.
- [#657](https://github.com/TokTok/c-toxcore/pull/657) Zero-initialise stack-allocated objects in hstox driver.
- [#656](https://github.com/TokTok/c-toxcore/pull/656) Fix file descriptor leak in hstox test.
- [#652](https://github.com/TokTok/c-toxcore/pull/652) Add support for building the monolith test on android.
- [#650](https://github.com/TokTok/c-toxcore/pull/650) Remove deprecated ToxDNS
- [#648](https://github.com/TokTok/c-toxcore/pull/648) Make hstox compile on FreeBSD
- [#624](https://github.com/TokTok/c-toxcore/pull/624) Update rpm spec and use variables in cmake instead of hardcoded paths
- [#616](https://github.com/TokTok/c-toxcore/pull/616) Add projects link to Readme.
- [#613](https://github.com/TokTok/c-toxcore/pull/613) Fix travis
- [#605](https://github.com/TokTok/c-toxcore/pull/605) Fix OS X Travis.
- [#598](https://github.com/TokTok/c-toxcore/pull/598) Fix typos in docs
- [#578](https://github.com/TokTok/c-toxcore/pull/578) Split toxav_bit_rate_set() into two functions to hold the maximum bitrates libvpx supports
- [#477](https://github.com/TokTok/c-toxcore/pull/477) Update install instructions to use CMake
- [#465](https://github.com/TokTok/c-toxcore/pull/465) Add Alpine linux Dockerfile in addition to the existing Debian one
- [#442](https://github.com/TokTok/c-toxcore/pull/442) Generate only one large library "libtoxcore".
- [#334](https://github.com/TokTok/c-toxcore/pull/334) Change toxencryptsave API to never overwrite pass keys.

### Closed issues:

- [#810](https://github.com/TokTok/c-toxcore/issues/810) Release 0.2.0
- [#704](https://github.com/TokTok/c-toxcore/issues/704) Add CORRECTION support to group chats
- [#620](https://github.com/TokTok/c-toxcore/issues/620) Video bug: large video frames are not sent correctly
- [#606](https://github.com/TokTok/c-toxcore/issues/606) groupId is int whereas friendId is uint32_t, reason?
- [#572](https://github.com/TokTok/c-toxcore/issues/572) int32_t may be not large enough as a argument for video_bit_rate of vp8/9 codec
- [#566](https://github.com/TokTok/c-toxcore/issues/566) LAYER #: modules for static linking - build issue
- [#42](https://github.com/TokTok/c-toxcore/issues/42) Remove ToxDNS and related stuff from toxcore

## v0.1.11

### Merged PRs:

- [#643](https://github.com/TokTok/c-toxcore/pull/643) Add .editorconfig
- [#638](https://github.com/TokTok/c-toxcore/pull/638) Release v0.1.11
- [#637](https://github.com/TokTok/c-toxcore/pull/637) Update tox-bootstrapd Dockerfile
- [#635](https://github.com/TokTok/c-toxcore/pull/635) Separate FreeBSD Travis build in 2 stages
- [#632](https://github.com/TokTok/c-toxcore/pull/632) Lift libconfig to v1.7.1
- [#631](https://github.com/TokTok/c-toxcore/pull/631) Add aspcud for Opam
- [#630](https://github.com/TokTok/c-toxcore/pull/630) Fix for Travis fail on addr_resolve testing
- [#623](https://github.com/TokTok/c-toxcore/pull/623) Split video payload into multiple RTP messages when too big to fit into one
- [#615](https://github.com/TokTok/c-toxcore/pull/615) forget DHT pubkey of offline friend after DHT timeout
- [#611](https://github.com/TokTok/c-toxcore/pull/611) Fix typo
- [#607](https://github.com/TokTok/c-toxcore/pull/607) set onion pingid timeout to announce timeout (300s)
- [#592](https://github.com/TokTok/c-toxcore/pull/592) Adjust docs of few toxencrypt function to the code
- [#587](https://github.com/TokTok/c-toxcore/pull/587) Fix tox test
- [#586](https://github.com/TokTok/c-toxcore/pull/586) Improve LAN discovery
- [#576](https://github.com/TokTok/c-toxcore/pull/576) Replace include(CTest) on enable_testing()
- [#574](https://github.com/TokTok/c-toxcore/pull/574) Reset hole-punching parameters after not punching for a while
- [#571](https://github.com/TokTok/c-toxcore/pull/571) Configure needs to find libsodium headers.
- [#515](https://github.com/TokTok/c-toxcore/pull/515) Network cleanup: reduce dependency on system-defined constants
- [#505](https://github.com/TokTok/c-toxcore/pull/505) Add FreeBSD Travis
- [#500](https://github.com/TokTok/c-toxcore/pull/500) Fixed the bug when receipts for messages sent from the receipt callback never arrived.

### Closed issues:

- [#493](https://github.com/TokTok/c-toxcore/issues/493) Receipts for messages sent from the receipt callback never arrive

## v0.1.10

### Merged PRs:

- [#575](https://github.com/TokTok/c-toxcore/pull/575) Release v0.1.10
- [#564](https://github.com/TokTok/c-toxcore/pull/564) Fix Windows build
- [#542](https://github.com/TokTok/c-toxcore/pull/542) Save bandwidth by moderating onion pinging

## v0.1.9

### Merged PRs:

- [#563](https://github.com/TokTok/c-toxcore/pull/563) Release v0.1.9
- [#561](https://github.com/TokTok/c-toxcore/pull/561) Remove unused variable
- [#560](https://github.com/TokTok/c-toxcore/pull/560) Fix non-portable zeroing out of doubles
- [#559](https://github.com/TokTok/c-toxcore/pull/559) Fix theoretical memory leaks
- [#557](https://github.com/TokTok/c-toxcore/pull/557) Document inverted mutex lock/unlock.
- [#556](https://github.com/TokTok/c-toxcore/pull/556) Build tests on appveyor, the MSVC build, but don't run them yet.
- [#555](https://github.com/TokTok/c-toxcore/pull/555) Fold hstox tests into the general linux test.
- [#554](https://github.com/TokTok/c-toxcore/pull/554) Add a monolith_test that includes all toxcore sources.
- [#553](https://github.com/TokTok/c-toxcore/pull/553) Factor out strict_abi cmake code into a separate module.
- [#552](https://github.com/TokTok/c-toxcore/pull/552) Fix formatting and spelling in version-sync script.
- [#551](https://github.com/TokTok/c-toxcore/pull/551) Forbid undefined symbols in shared libraries.
- [#546](https://github.com/TokTok/c-toxcore/pull/546) Make variable names in file saving test less cryptic
- [#539](https://github.com/TokTok/c-toxcore/pull/539) Make OSX test failures fail the Travis CI build.
- [#537](https://github.com/TokTok/c-toxcore/pull/537) Fix TokTok/c-toxcore#535
- [#534](https://github.com/TokTok/c-toxcore/pull/534) Fix markdown formatting
- [#530](https://github.com/TokTok/c-toxcore/pull/530) Implement missing TES constant functions.
- [#511](https://github.com/TokTok/c-toxcore/pull/511) Save bandwidth by avoiding superfluous Nodes Requests to peers already on the Close List
- [#506](https://github.com/TokTok/c-toxcore/pull/506) Add test case for title change
- [#498](https://github.com/TokTok/c-toxcore/pull/498) DHT refactoring
- [#487](https://github.com/TokTok/c-toxcore/pull/487) Split daemon's logging backends in separate modules
- [#468](https://github.com/TokTok/c-toxcore/pull/468) Test for memberlist not changing after changing own name
- [#449](https://github.com/TokTok/c-toxcore/pull/449) Use new encoding of `Maybe` in msgpack results.

### Closed issues:

- [#482](https://github.com/TokTok/c-toxcore/issues/482) CMake can't detect and compile ToxAV on OSX

## v0.1.8

### Merged PRs:

- [#538](https://github.com/TokTok/c-toxcore/pull/538) Reverting tox_loop PR changes
- [#536](https://github.com/TokTok/c-toxcore/pull/536) Release v0.1.8
- [#526](https://github.com/TokTok/c-toxcore/pull/526) Add TOX_NOSPAM_SIZE to the public API.
- [#525](https://github.com/TokTok/c-toxcore/pull/525) Retry autotools tests the same way as cmake tests.
- [#524](https://github.com/TokTok/c-toxcore/pull/524) Reduce ctest timeout to 2 minutes from 5 minutes.
- [#512](https://github.com/TokTok/c-toxcore/pull/512) Add test for DHT pack_nodes and unpack_nodes
- [#504](https://github.com/TokTok/c-toxcore/pull/504) CMake: install bootstrapd if it is built
- [#488](https://github.com/TokTok/c-toxcore/pull/488) Save compiled Android artifacts after CircleCI builds.
- [#473](https://github.com/TokTok/c-toxcore/pull/473) Added missing includes: <netinet/in.h> and <sys/socket.h>
- [#335](https://github.com/TokTok/c-toxcore/pull/335) Implement tox_loop

### Closed issues:

- [#535](https://github.com/TokTok/c-toxcore/issues/535) OS X tests failing
- [#503](https://github.com/TokTok/c-toxcore/issues/503) Undefined functions: tox_pass_salt_length, tox_pass_key_length, tox_pass_encryption_extra_length
- [#456](https://github.com/TokTok/c-toxcore/issues/456) Tox.h doesn't expose the size of the nospam.
- [#411](https://github.com/TokTok/c-toxcore/issues/411) Reduce CTest timeout to 2 minutes

## v0.1.7

### Merged PRs:

- [#523](https://github.com/TokTok/c-toxcore/pull/523) Release v0.1.7
- [#521](https://github.com/TokTok/c-toxcore/pull/521) Fix appveyor script: install curl from chocolatey.
- [#510](https://github.com/TokTok/c-toxcore/pull/510) Fix list malloc(0) bug
- [#509](https://github.com/TokTok/c-toxcore/pull/509) Fix network malloc(0) bug
- [#497](https://github.com/TokTok/c-toxcore/pull/497) Fix network
- [#496](https://github.com/TokTok/c-toxcore/pull/496) Fix Travis always succeeding despite tests failing
- [#491](https://github.com/TokTok/c-toxcore/pull/491) Add crypto_memzero for temp buffer
- [#490](https://github.com/TokTok/c-toxcore/pull/490) Move c_sleep to helpers.h and misc_tools.h
- [#486](https://github.com/TokTok/c-toxcore/pull/486) Remove empty line in Messenger.c
- [#483](https://github.com/TokTok/c-toxcore/pull/483) Make BUILD_TOXAV an option and fail if dependencies are missing
- [#481](https://github.com/TokTok/c-toxcore/pull/481) Remove dependency on strings.h
- [#480](https://github.com/TokTok/c-toxcore/pull/480) Use VLA macro
- [#479](https://github.com/TokTok/c-toxcore/pull/479) Fix pthreads in AppVeyor build
- [#471](https://github.com/TokTok/c-toxcore/pull/471) Remove statics used in onion comparison functions.
- [#461](https://github.com/TokTok/c-toxcore/pull/461) Replace part of network functions on platform-independent implementation
- [#452](https://github.com/TokTok/c-toxcore/pull/452) Add VLA compatibility macro for C89-ish compilers.

### Closed issues:

- [#474](https://github.com/TokTok/c-toxcore/issues/474) TOX_VERSION_PATCH isn't in sync with the version

## v0.1.6

### Merged PRs:

- [#460](https://github.com/TokTok/c-toxcore/pull/460) Release v0.1.6.
- [#459](https://github.com/TokTok/c-toxcore/pull/459) Add Android build to CI.
- [#454](https://github.com/TokTok/c-toxcore/pull/454) Add appveyor build for native windows tests.
- [#448](https://github.com/TokTok/c-toxcore/pull/448) Only retry failed tests on Circle CI instead of all.
- [#434](https://github.com/TokTok/c-toxcore/pull/434) Replace redundant packet type check in handler with assert.
- [#432](https://github.com/TokTok/c-toxcore/pull/432) Remove some static variables
- [#385](https://github.com/TokTok/c-toxcore/pull/385) Add platform-independent Socket and IP implementation

### Closed issues:

- [#415](https://github.com/TokTok/c-toxcore/issues/415) Set up a native windows build on appveyor

## v0.1.5

### Merged PRs:

- [#447](https://github.com/TokTok/c-toxcore/pull/447) Release v0.1.5.
- [#446](https://github.com/TokTok/c-toxcore/pull/446) Limit number of retries to 3.
- [#445](https://github.com/TokTok/c-toxcore/pull/445) Make Travis tests slightly more robust by re-running them.
- [#443](https://github.com/TokTok/c-toxcore/pull/443) Make building `DHT_bootstrap` in cmake optional.
- [#433](https://github.com/TokTok/c-toxcore/pull/433) Add tutorial and "danger: experimental" banner to README.
- [#431](https://github.com/TokTok/c-toxcore/pull/431) Update license headers and remove redundant file name comment.
- [#424](https://github.com/TokTok/c-toxcore/pull/424) Fixed the FreeBSD build failure due to the undefined MSG_NOSIGNAL.
- [#420](https://github.com/TokTok/c-toxcore/pull/420) Setup autotools to read .so version info from a separate file
- [#418](https://github.com/TokTok/c-toxcore/pull/418) Clarify how the autotools build is done on Travis.
- [#414](https://github.com/TokTok/c-toxcore/pull/414) Explicitly check if compiler supports C99

## v0.1.4

### Merged PRs:

- [#422](https://github.com/TokTok/c-toxcore/pull/422) Release v0.1.4.
- [#410](https://github.com/TokTok/c-toxcore/pull/410) Fix NaCl build: tar was called incorrectly.
- [#409](https://github.com/TokTok/c-toxcore/pull/409) Clarify that the pass key `new` function can fail.
- [#407](https://github.com/TokTok/c-toxcore/pull/407) Don't use `git.depth=1` anymore.
- [#404](https://github.com/TokTok/c-toxcore/pull/404) Issue 404: semicolon not found
- [#403](https://github.com/TokTok/c-toxcore/pull/403) Warn on -pedantic, don't error yet.
- [#401](https://github.com/TokTok/c-toxcore/pull/401) Add logging callback to messenger_test.
- [#400](https://github.com/TokTok/c-toxcore/pull/400) Run windows tests but ignore their failures.
- [#398](https://github.com/TokTok/c-toxcore/pull/398) Portability Fixes
- [#397](https://github.com/TokTok/c-toxcore/pull/397) Replace make_quick_sort with qsort
- [#396](https://github.com/TokTok/c-toxcore/pull/396) Add an OSX build that doesn't run tests.
- [#394](https://github.com/TokTok/c-toxcore/pull/394) CMake: Add soversion to library files to generate proper symlinks
- [#393](https://github.com/TokTok/c-toxcore/pull/393) Set up autotools build to build against vanilla NaCl.
- [#392](https://github.com/TokTok/c-toxcore/pull/392) Check that TCP connections aren't dropped in callbacks.
- [#391](https://github.com/TokTok/c-toxcore/pull/391) Minor simplification in `file_seek` code.
- [#390](https://github.com/TokTok/c-toxcore/pull/390) Always kill invalid file transfers when receiving file controls.
- [#388](https://github.com/TokTok/c-toxcore/pull/388) Fix logging condition for IPv6 client timestamp updates.
- [#387](https://github.com/TokTok/c-toxcore/pull/387) Eliminate dead return statement.
- [#386](https://github.com/TokTok/c-toxcore/pull/386) Avoid accessing uninitialised memory in `net_crypto`.
- [#381](https://github.com/TokTok/c-toxcore/pull/381) Remove `TOX_DEBUG` and have asserts always enabled.

### Closed issues:

- [#378](https://github.com/TokTok/c-toxcore/issues/378) Replace all uses of `make_quick_sort` with `qsort`
- [#364](https://github.com/TokTok/c-toxcore/issues/364) Delete misc_tools.h after replacing its use by qsort.
- [#363](https://github.com/TokTok/c-toxcore/issues/363) Test against NaCl in addition to libsodium on Travis.

## v0.1.3

### Merged PRs:

- [#395](https://github.com/TokTok/c-toxcore/pull/395) Revert "Portability fixes"
- [#380](https://github.com/TokTok/c-toxcore/pull/380) Test a few cmake option combinations before the build.
- [#377](https://github.com/TokTok/c-toxcore/pull/377) Fix SSL verification in coveralls.
- [#376](https://github.com/TokTok/c-toxcore/pull/376) Bring back autotools instructions
- [#373](https://github.com/TokTok/c-toxcore/pull/373) Only fetch 1 revision from git during Travis builds.
- [#369](https://github.com/TokTok/c-toxcore/pull/369) Integrate with CircleCI to build artifacts in the future
- [#366](https://github.com/TokTok/c-toxcore/pull/366) Release v0.1.3.
- [#362](https://github.com/TokTok/c-toxcore/pull/362) Remove .cabal-sandbox option from tox-spectest find line.
- [#361](https://github.com/TokTok/c-toxcore/pull/361) Simplify integration as a third-party lib in cmake projects
- [#354](https://github.com/TokTok/c-toxcore/pull/354) Add secure memcmp and memzero implementation.
- [#324](https://github.com/TokTok/c-toxcore/pull/324) Do not compile and install DHT_bootstrap if it was disabled in configure
- [#297](https://github.com/TokTok/c-toxcore/pull/297) Portability fixes

### Closed issues:

- [#347](https://github.com/TokTok/c-toxcore/issues/347) Implement our own secure `memcmp` and `memzero` if libsodium isn't available
- [#319](https://github.com/TokTok/c-toxcore/issues/319) toxcore installs `DHT_bootstrap` even though `--disable-daemon` is passed to `./configure`

## v0.1.2

### Merged PRs:

- [#355](https://github.com/TokTok/c-toxcore/pull/355) Release v0.1.2
- [#353](https://github.com/TokTok/c-toxcore/pull/353) Fix toxav use after free caused by premature MSI destruction
- [#346](https://github.com/TokTok/c-toxcore/pull/346) Avoid array out of bounds read in friend saving.
- [#344](https://github.com/TokTok/c-toxcore/pull/344) Remove unused get/set salt/key functions from toxencryptsave.
- [#343](https://github.com/TokTok/c-toxcore/pull/343) Wrap all sodium/nacl functions in crypto_core.c.
- [#341](https://github.com/TokTok/c-toxcore/pull/341) Add test to check if tox_new/tox_kill leaks.
- [#336](https://github.com/TokTok/c-toxcore/pull/336) Correct TES docs to reflect how many bytes functions actually require.
- [#333](https://github.com/TokTok/c-toxcore/pull/333) Use `tox_options_set_*` instead of direct member access.

### Closed issues:

- [#345](https://github.com/TokTok/c-toxcore/issues/345) Array out of bounds read in "save" function
- [#342](https://github.com/TokTok/c-toxcore/issues/342) Wrap all libsodium functions we use in toxcore in `crypto_core`.
- [#278](https://github.com/TokTok/c-toxcore/issues/278) ToxAV use-after-free bug

## v0.1.1

### Merged PRs:

- [#337](https://github.com/TokTok/c-toxcore/pull/337) Release v0.1.1
- [#332](https://github.com/TokTok/c-toxcore/pull/332) Add test for encrypted savedata.
- [#330](https://github.com/TokTok/c-toxcore/pull/330) Strengthen the note about ABI compatibility in tox.h.
- [#328](https://github.com/TokTok/c-toxcore/pull/328) Drop the broken `TOX_VERSION_REQUIRE` macro.
- [#326](https://github.com/TokTok/c-toxcore/pull/326) Fix unresolved reference in toxencryptsave API docs.
- [#309](https://github.com/TokTok/c-toxcore/pull/309) Fixed attempt to join detached threads (fixes toxav test crash)
- [#306](https://github.com/TokTok/c-toxcore/pull/306) Add option to disable local peer discovery

### Closed issues:

- [#327](https://github.com/TokTok/c-toxcore/issues/327) The `TOX_VERSION_REQUIRE` macro is broken.
- [#221](https://github.com/TokTok/c-toxcore/issues/221) Option to disable local peer detection

## v0.1.0

### Merged PRs:

- [#325](https://github.com/TokTok/c-toxcore/pull/325) Fix Libs line in toxcore.pc pkg-config file.
- [#322](https://github.com/TokTok/c-toxcore/pull/322) Add compatibility pkg-config modules: libtoxcore, libtoxav.
- [#318](https://github.com/TokTok/c-toxcore/pull/318) Fix `--enable-logging` flag in autotools configure script.
- [#316](https://github.com/TokTok/c-toxcore/pull/316) Release 0.1.0.
- [#315](https://github.com/TokTok/c-toxcore/pull/315) Fix version compatibility test.
- [#314](https://github.com/TokTok/c-toxcore/pull/314) Fix off by one error in saving our own status message.
- [#313](https://github.com/TokTok/c-toxcore/pull/313) Fix padding being in the wrong place in `SAVED_FRIEND` struct
- [#312](https://github.com/TokTok/c-toxcore/pull/312) Conditionally enable non-portable assert on LP64.
- [#310](https://github.com/TokTok/c-toxcore/pull/310) Add apidsl file for toxencryptsave.
- [#307](https://github.com/TokTok/c-toxcore/pull/307) Clarify toxencryptsave documentation regarding buffer sizes
- [#305](https://github.com/TokTok/c-toxcore/pull/305) Fix static builds
- [#303](https://github.com/TokTok/c-toxcore/pull/303) Don't build nTox by default.
- [#301](https://github.com/TokTok/c-toxcore/pull/301) Renamed messenger functions, prepend `m_`.
- [#299](https://github.com/TokTok/c-toxcore/pull/299) net_crypto give handle_data_packet_helper a better name
- [#294](https://github.com/TokTok/c-toxcore/pull/294) Don't error on warnings by default

### Closed issues:

- [#317](https://github.com/TokTok/c-toxcore/issues/317) toxcore fails to build with autotools and debugging level enabled
- [#311](https://github.com/TokTok/c-toxcore/issues/311) Incorrect padding
- [#308](https://github.com/TokTok/c-toxcore/issues/308) Review TES and port it to APIDSL
- [#293](https://github.com/TokTok/c-toxcore/issues/293) error building on ubuntu 14.04
- [#292](https://github.com/TokTok/c-toxcore/issues/292) Don't build nTox by default with CMake
- [#290](https://github.com/TokTok/c-toxcore/issues/290) User Feed
- [#266](https://github.com/TokTok/c-toxcore/issues/266) Support all levels listed in TOX_DHT_NAT_LEVEL
- [#216](https://github.com/TokTok/c-toxcore/issues/216) When v0.1 release?

## v0.0.5

### Merged PRs:

- [#289](https://github.com/TokTok/c-toxcore/pull/289) Version Patch v0.0.4 => v0.0.5
- [#287](https://github.com/TokTok/c-toxcore/pull/287) Add CMake knobs to suppress building tests
- [#286](https://github.com/TokTok/c-toxcore/pull/286) Support float32 and float64 in msgpack type printer.
- [#285](https://github.com/TokTok/c-toxcore/pull/285) Mark `Tox_Options` struct as deprecated.
- [#284](https://github.com/TokTok/c-toxcore/pull/284) Add NONE enumerator to bit mask.
- [#281](https://github.com/TokTok/c-toxcore/pull/281) Made save format platform-independent
- [#277](https://github.com/TokTok/c-toxcore/pull/277) Fix a memory leak in hstox interface
- [#276](https://github.com/TokTok/c-toxcore/pull/276) Fix NULL pointer dereference in log calls
- [#275](https://github.com/TokTok/c-toxcore/pull/275) Fix a memory leak in GroupAV
- [#274](https://github.com/TokTok/c-toxcore/pull/274) Options in `new_messenger()` must never be null.
- [#271](https://github.com/TokTok/c-toxcore/pull/271) Convert to and from network byte order in set/get nospam.
- [#262](https://github.com/TokTok/c-toxcore/pull/262) Add ability to disable UDP hole punching

### Closed issues:

- [#254](https://github.com/TokTok/c-toxcore/issues/254) Add option to disable UDP hole punching
- [#215](https://github.com/TokTok/c-toxcore/issues/215) The current tox save format is non-portable
- [#205](https://github.com/TokTok/c-toxcore/issues/205) nospam value is reversed in array returned by `tox_self_get_address()`

## v0.0.4

### Merged PRs:

- [#272](https://github.com/TokTok/c-toxcore/pull/272) v0.0.4
- [#265](https://github.com/TokTok/c-toxcore/pull/265) Disable -Wunused-but-set-variable compiler warning flag.
- [#261](https://github.com/TokTok/c-toxcore/pull/261) Work around Travis issue that causes build failures.
- [#260](https://github.com/TokTok/c-toxcore/pull/260) Support arbitrary video resolutions in av_test
- [#257](https://github.com/TokTok/c-toxcore/pull/257) Add decode/encode PlainText test support.
- [#256](https://github.com/TokTok/c-toxcore/pull/256) Add spectest to the cmake test suite.
- [#255](https://github.com/TokTok/c-toxcore/pull/255) Disable some gcc-specific warnings.
- [#249](https://github.com/TokTok/c-toxcore/pull/249) Use apidsl for the crypto_core API.
- [#248](https://github.com/TokTok/c-toxcore/pull/248) Remove new_nonce function in favour of random_nonce.
- [#224](https://github.com/TokTok/c-toxcore/pull/224) Add DHT_create_packet, an abstraction for DHT RPC packets

## v0.0.3

### Merged PRs:

- [#251](https://github.com/TokTok/c-toxcore/pull/251) Rename log levels to remove the extra "LOG" prefix.
- [#250](https://github.com/TokTok/c-toxcore/pull/250) Release v0.0.3.
- [#245](https://github.com/TokTok/c-toxcore/pull/245) Change packet kind enum to use hex constants.
- [#243](https://github.com/TokTok/c-toxcore/pull/243) Enable address sanitizer on the cmake build.
- [#242](https://github.com/TokTok/c-toxcore/pull/242) Remove assoc
- [#241](https://github.com/TokTok/c-toxcore/pull/241) Move log callback to options.
- [#233](https://github.com/TokTok/c-toxcore/pull/233) Enable all possible C compiler warning flags.
- [#230](https://github.com/TokTok/c-toxcore/pull/230) Move packing and unpacking DHT request packets to DHT module.
- [#228](https://github.com/TokTok/c-toxcore/pull/228) Remove unimplemented "time delta" parameter.
- [#227](https://github.com/TokTok/c-toxcore/pull/227) Compile as C++ for windows builds.
- [#223](https://github.com/TokTok/c-toxcore/pull/223) TravisCI shorten IRC message
- [#220](https://github.com/TokTok/c-toxcore/pull/220) toxav renaming: group.{h,c} -> groupav.{h,c}
- [#218](https://github.com/TokTok/c-toxcore/pull/218) Rename some internal "group chat" thing to "conference".
- [#212](https://github.com/TokTok/c-toxcore/pull/212) Convert series of `NET_PACKET_*` defines into a typedef enum
- [#196](https://github.com/TokTok/c-toxcore/pull/196) Update readme, moved the roadmap to a higher position
- [#193](https://github.com/TokTok/c-toxcore/pull/193) Remove duplicate tests: split tests part 2.

### Closed issues:

- [#40](https://github.com/TokTok/c-toxcore/issues/40) Stateless callbacks in toxcore's public API

## v0.0.2

### Merged PRs:

- [#207](https://github.com/TokTok/c-toxcore/pull/207) docs: correct instructions for cloning & harden against repo name changes
- [#206](https://github.com/TokTok/c-toxcore/pull/206) Corrected libsodium tag
- [#204](https://github.com/TokTok/c-toxcore/pull/204) Error if format_test can't be executed.
- [#202](https://github.com/TokTok/c-toxcore/pull/202) Version Patch v0.0.2
- [#190](https://github.com/TokTok/c-toxcore/pull/190) Install libraries with RPATH.
- [#189](https://github.com/TokTok/c-toxcore/pull/189) Use `socklen_t` instead of `unsigned int` in call to `accept`.
- [#188](https://github.com/TokTok/c-toxcore/pull/188) Add option to set test timeout
- [#187](https://github.com/TokTok/c-toxcore/pull/187) Add option to build tox-bootstrapd
- [#185](https://github.com/TokTok/c-toxcore/pull/185) Import the hstox SUT interface from hstox.
- [#183](https://github.com/TokTok/c-toxcore/pull/183) Set log level for DEBUG=ON to LOG_DEBUG.
- [#182](https://github.com/TokTok/c-toxcore/pull/182) Remove return after no-return situation.
- [#181](https://github.com/TokTok/c-toxcore/pull/181) Minor documentation fixes.
- [#180](https://github.com/TokTok/c-toxcore/pull/180) Add the 'Tox' context object to the logger.
- [#179](https://github.com/TokTok/c-toxcore/pull/179) Remove the `_test` suffix in `auto_test` calls.
- [#178](https://github.com/TokTok/c-toxcore/pull/178) Rebuild apidsl'd headers in cmake.
- [#177](https://github.com/TokTok/c-toxcore/pull/177) docs(INSTALL): update compiling instructions for Linux
- [#176](https://github.com/TokTok/c-toxcore/pull/176) Merge irungentoo/toxcore into TokTok/c-toxcore.
- [#173](https://github.com/TokTok/c-toxcore/pull/173) Duplicate tox_test to 4 other files.

### Closed issues:

- [#201](https://github.com/TokTok/c-toxcore/issues/201) Logging callback was broken

## v0.0.1

### Merged PRs:

- [#174](https://github.com/TokTok/c-toxcore/pull/174) Remove redundant callback objects.
- [#171](https://github.com/TokTok/c-toxcore/pull/171) Simple Version tick to v0.0.1
- [#170](https://github.com/TokTok/c-toxcore/pull/170) C++ the second round.
- [#166](https://github.com/TokTok/c-toxcore/pull/166) Add version-sync script.
- [#164](https://github.com/TokTok/c-toxcore/pull/164) Replace `void*` with `RingBuffer*` to avoid conversions.
- [#163](https://github.com/TokTok/c-toxcore/pull/163) Move ring buffer out of toxcore/util into toxav.
- [#162](https://github.com/TokTok/c-toxcore/pull/162) Allow the OSX build to fail on travis.
- [#161](https://github.com/TokTok/c-toxcore/pull/161) Minor cleanups: unused vars, unreachable code, static globals.
- [#160](https://github.com/TokTok/c-toxcore/pull/160) Work around bug in opencv3 headers.
- [#157](https://github.com/TokTok/c-toxcore/pull/157) Make TCP_Connections module-private.
- [#156](https://github.com/TokTok/c-toxcore/pull/156) Make TCP_Server opaque.
- [#153](https://github.com/TokTok/c-toxcore/pull/153) Fix strict-ld grep expressions to include digits.
- [#151](https://github.com/TokTok/c-toxcore/pull/151) Revert #130 "Make ToxAV stateless"
- [#148](https://github.com/TokTok/c-toxcore/pull/148) Added UB comment r/t deleting a friend w/ active call
- [#146](https://github.com/TokTok/c-toxcore/pull/146) Make group callbacks stateless
- [#145](https://github.com/TokTok/c-toxcore/pull/145) Make internal chat list function take uint32_t* as well.
- [#144](https://github.com/TokTok/c-toxcore/pull/144) Only build toxav if opus and vpx are found.
- [#143](https://github.com/TokTok/c-toxcore/pull/143) Make toxcore code C++ compatible.
- [#142](https://github.com/TokTok/c-toxcore/pull/142) Fix for windows dynamic libraries.
- [#141](https://github.com/TokTok/c-toxcore/pull/141) const-correctness in windows code.
- [#140](https://github.com/TokTok/c-toxcore/pull/140) Use C99 %zu format conversion in printf for size_t.
- [#139](https://github.com/TokTok/c-toxcore/pull/139) Clean up Travis build a bit in preparation for osx/win.
- [#138](https://github.com/TokTok/c-toxcore/pull/138) Remove format-source from travis script.
- [#135](https://github.com/TokTok/c-toxcore/pull/135) Convert old groupchats to new API format
- [#134](https://github.com/TokTok/c-toxcore/pull/134) Add some astyle options to make it do more.
- [#133](https://github.com/TokTok/c-toxcore/pull/133) Ensure that all TODOs have an owner.
- [#132](https://github.com/TokTok/c-toxcore/pull/132) Remove `else` directly after `return`.
- [#130](https://github.com/TokTok/c-toxcore/pull/130) Make ToxAV stateless
- [#129](https://github.com/TokTok/c-toxcore/pull/129) Use TokTok's apidsl instead of the iphydf one.
- [#127](https://github.com/TokTok/c-toxcore/pull/127) Use "phase" script for travis build phases.
- [#126](https://github.com/TokTok/c-toxcore/pull/126) Add option to build static libraries.
- [#125](https://github.com/TokTok/c-toxcore/pull/125) Group #include directives in 3-4 groups.
- [#123](https://github.com/TokTok/c-toxcore/pull/123) Use correct logical operator for tox_test
- [#120](https://github.com/TokTok/c-toxcore/pull/120) make the majority of the callbacks stateless and add some status to a testcase
- [#118](https://github.com/TokTok/c-toxcore/pull/118) Use `const` for version numbers.
- [#117](https://github.com/TokTok/c-toxcore/pull/117) Add STRICT_ABI cmake flag to generate export lists.
- [#116](https://github.com/TokTok/c-toxcore/pull/116) Fix potential null pointer dereference.
- [#115](https://github.com/TokTok/c-toxcore/pull/115) Fix memory leak on error paths in tox_new.
- [#114](https://github.com/TokTok/c-toxcore/pull/114) Fix compilation for Windows.
- [#111](https://github.com/TokTok/c-toxcore/pull/111) Add debugging option to autotools configuration
- [#110](https://github.com/TokTok/c-toxcore/pull/110) Comment intentional switch fallthroughs
- [#109](https://github.com/TokTok/c-toxcore/pull/109) Separate ip_port packing from pack_nodes() and unpack_nodes()
- [#108](https://github.com/TokTok/c-toxcore/pull/108) Prevent `<winsock.h>` inclusion by `<windows.h>`.
- [#107](https://github.com/TokTok/c-toxcore/pull/107) Print a message about missing astyle in format-source.
- [#104](https://github.com/TokTok/c-toxcore/pull/104) Merge with irungentoo/master
- [#103](https://github.com/TokTok/c-toxcore/pull/103) Allocate `sizeof(IP_ADAPTER_INFO)` bytes instead of `sizeof(T*)`.
- [#101](https://github.com/TokTok/c-toxcore/pull/101) Add TODO for @mannol.
- [#100](https://github.com/TokTok/c-toxcore/pull/100) Remove the packet mutation in toxav's bwcontroller.
- [#99](https://github.com/TokTok/c-toxcore/pull/99) Make packet data a ptr-to-const.
- [#97](https://github.com/TokTok/c-toxcore/pull/97) Improve static and const correctness.
- [#96](https://github.com/TokTok/c-toxcore/pull/96) Improve C standard compliance.
- [#94](https://github.com/TokTok/c-toxcore/pull/94) Rearrange fields to decrease size of structure
- [#84](https://github.com/TokTok/c-toxcore/pull/84) Remove useless casts.
- [#82](https://github.com/TokTok/c-toxcore/pull/82) Add missing #include <pthread.h> to av_test.c.
- [#81](https://github.com/TokTok/c-toxcore/pull/81) Match parameter names in declarations with their definitions.
- [#80](https://github.com/TokTok/c-toxcore/pull/80) Sort #includes in all source files.
- [#79](https://github.com/TokTok/c-toxcore/pull/79) Remove redundant `return` statements.
- [#78](https://github.com/TokTok/c-toxcore/pull/78) Do not use `else` after `return`.
- [#77](https://github.com/TokTok/c-toxcore/pull/77) Add OSX and Windows build to travis config.
- [#76](https://github.com/TokTok/c-toxcore/pull/76) Remove unused and bit-rotten friends_test.
- [#75](https://github.com/TokTok/c-toxcore/pull/75) Enable build of av_test.
- [#74](https://github.com/TokTok/c-toxcore/pull/74) Add missing #includes to headers and rename tox_old to tox_group.
- [#73](https://github.com/TokTok/c-toxcore/pull/73) Add braces to all if statements.
- [#72](https://github.com/TokTok/c-toxcore/pull/72) Add getters/setters for options.
- [#70](https://github.com/TokTok/c-toxcore/pull/70) Expose constants as functions.
- [#68](https://github.com/TokTok/c-toxcore/pull/68) Add address sanitizer option to cmake file.
- [#66](https://github.com/TokTok/c-toxcore/pull/66) Fix plane size calculation in test
- [#65](https://github.com/TokTok/c-toxcore/pull/65) Avoid large stack allocations on thread stacks.
- [#64](https://github.com/TokTok/c-toxcore/pull/64) Comment out useless TODO'd if block.
- [#63](https://github.com/TokTok/c-toxcore/pull/63) Initialise the id in assoc_test.
- [#62](https://github.com/TokTok/c-toxcore/pull/62) Reduce the timeout on travis to something much more reasonable
- [#60](https://github.com/TokTok/c-toxcore/pull/60) Make friend requests stateless
- [#59](https://github.com/TokTok/c-toxcore/pull/59) Replace uint with unsigned int in assoc.c.
- [#58](https://github.com/TokTok/c-toxcore/pull/58) Make Message received receipts stateless
- [#57](https://github.com/TokTok/c-toxcore/pull/57) Make Friend User Status stateless
- [#55](https://github.com/TokTok/c-toxcore/pull/55) docs(INSTALL.md): update instructions for Gentoo
- [#54](https://github.com/TokTok/c-toxcore/pull/54) Make typing change callback stateless
- [#53](https://github.com/TokTok/c-toxcore/pull/53) Add format-source script.
- [#52](https://github.com/TokTok/c-toxcore/pull/52) Build assoc DHT code on travis.
- [#51](https://github.com/TokTok/c-toxcore/pull/51) Fix operation sequencing in TCP_test.
- [#49](https://github.com/TokTok/c-toxcore/pull/49) Apidsl test
- [#48](https://github.com/TokTok/c-toxcore/pull/48) Make friend message callback stateless
- [#46](https://github.com/TokTok/c-toxcore/pull/46) Move logging to a callback.
- [#45](https://github.com/TokTok/c-toxcore/pull/45) Stateless friend status message
- [#43](https://github.com/TokTok/c-toxcore/pull/43) Allow NULL as argument to tox_kill.
- [#41](https://github.com/TokTok/c-toxcore/pull/41) Fix warnings
- [#39](https://github.com/TokTok/c-toxcore/pull/39) Merge irungentoo/toxcore into TokTok/c-toxcore.
- [#38](https://github.com/TokTok/c-toxcore/pull/38) Try searching for libsodium with pkg-config in ./configure.
- [#37](https://github.com/TokTok/c-toxcore/pull/37) Add missing DHT_bootstrap to CMakeLists.txt.
- [#36](https://github.com/TokTok/c-toxcore/pull/36) Make tox_callback_friend_name stateless.
- [#33](https://github.com/TokTok/c-toxcore/pull/33) Update readme with tentative roadmap, removed old todo.md
- [#32](https://github.com/TokTok/c-toxcore/pull/32) Fix a bug I introduced that would make toxcore fail to initialise a second time
- [#31](https://github.com/TokTok/c-toxcore/pull/31) 7. Travis envs
- [#30](https://github.com/TokTok/c-toxcore/pull/30) 2. Hstox test
- [#29](https://github.com/TokTok/c-toxcore/pull/29) 1. Move toxcore travis build scripts out of .travis.yml.
- [#27](https://github.com/TokTok/c-toxcore/pull/27) 8. Stateless
- [#26](https://github.com/TokTok/c-toxcore/pull/26) 6. Cmake bootstrapd
- [#25](https://github.com/TokTok/c-toxcore/pull/25) 5. Coverage clang
- [#24](https://github.com/TokTok/c-toxcore/pull/24) Silence/fix some compiler warnings.
- [#23](https://github.com/TokTok/c-toxcore/pull/23) 4. Cmake
- [#20](https://github.com/TokTok/c-toxcore/pull/20) 3. Travis astyle
- [#13](https://github.com/TokTok/c-toxcore/pull/13) Enable, and report test status
- [#12](https://github.com/TokTok/c-toxcore/pull/12) Fix readme for TokTok
- [#11](https://github.com/TokTok/c-toxcore/pull/11) Documentation: SysVInit workaround for <1024 ports
- [#2](https://github.com/TokTok/c-toxcore/pull/2) Enable toxcore logging when building on Travis.
- [#1](https://github.com/TokTok/c-toxcore/pull/1) Apidsl fixes and start tracking test coverage

### Closed issues:

- [#158](https://github.com/TokTok/c-toxcore/issues/158) Error while build with OpenCV 3.1
- [#147](https://github.com/TokTok/c-toxcore/issues/147) Add comment to m_delfriend about the NULL passing to the internal conn status cb
- [#136](https://github.com/TokTok/c-toxcore/issues/136) Replace astyle by clang-format
- [#113](https://github.com/TokTok/c-toxcore/issues/113) Toxcore tests fail
- [#83](https://github.com/TokTok/c-toxcore/issues/83) Travis tests are hard to quickly parse from their output.
- [#22](https://github.com/TokTok/c-toxcore/issues/22) Make the current tests exercise both ipv4 and ipv6.
- [#9](https://github.com/TokTok/c-toxcore/issues/9) Fix the failing test
- [#8](https://github.com/TokTok/c-toxcore/issues/8) Toxcore should make more liberal use of assertions
- [#4](https://github.com/TokTok/c-toxcore/issues/4) Integrate hstox tests with toxcore Travis build
