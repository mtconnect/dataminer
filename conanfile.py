from conans import ConanFile, CMake, tools

class DataminerConan(ConanFile):
    name = "Dataminer"
    version = "1.7"
    generators = "cmake"
    url = "https://github.com/mtconnect/dataminer.git"
    settings = "os", "compiler", "arch", "build_type", "arch_build"
    description = "Dataminer for MTConnect"
    
    requires = ["boost/1.77.0", "openssl/1.1.1k"]
    build_policy = "missing"
    default_options = {
        "boost:python_version": "3.9",
        "boost:python_executable": "python3",
        "boost:shared": False,
        "boost:bzip2": False,
        "boost:lzma": False,
        "boost:without_wave": True,
        "boost:without_test": True,
        "boost:without_json": False,
        "boost:without_mpi": True,
        "boost:without_stacktrace": True,
        "boost:extra_b2_flags": "-j 2 -d +1 cxxstd=17 cxxflags=-fvisibility=hidden ",
        "boost:i18n_backend_iconv": 'off',
        "boost:i18n_backend_icu": True
        }

    def configure(self):
        self.windows_xp = self.settings.os == 'Windows' and self.settings.compiler.toolset and \
                          self.settings.compiler.toolset in ('v141_xp', 'v140_xp')
        if self.settings.os == 'Windows':
            self.options['boost'].i18n_backend_icu = False
            self.options["boost"].extra_b2_flags = self.options["boost"].extra_b2_flags + \
                " boost.locale.icu=off boost.locale.iconv=off boost.locale.winapi=on asmflags=/safeseh "
            if self.settings.build_type and self.settings.build_type == 'Debug':
                self.settings.compiler.runtime = 'MTd'
            else:
                self.settings.compiler.runtime = 'MT'
            self.settings.compiler.version = '16'
        
        if "libcxx" in self.settings.compiler.fields and self.settings.compiler.libcxx == "libstdc++":
            raise Exception("This package is only compatible with libstdc++11, add -s compiler.libcxx=libstdc++11")
        
        self.settings.compiler.cppstd = 17

        if self.windows_xp:
            self.options["boost"].extra_b2_flags = self.options["boost"].extra_b2_flags + "define=BOOST_USE_WINAPI_VERSION=0x0501 "
        elif self.settings.os == 'Windows':
            self.options["boost"].extra_b2_flags = self.options["boost"].extra_b2_flags + "define=BOOST_USE_WINAPI_VERSION=0x0600 "            

    def build(self):
        cmake = CMake(self)

        if self.windows_xp:
            cmake.definitions['WINVER'] = '0x0501'

        cmake.definitions['CMAKE_BUILD_TYPE'] = self.settings.build_type
        cmake.configure()
        cmake.build()

    def imports(self):
        self.copy("*.dll", "bin", "bin")
        self.copy("*.so*", "bin", "lib")
        self.copy("*.dylib", "bin", "lib")

