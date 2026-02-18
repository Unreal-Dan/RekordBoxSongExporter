from conan import ConanFile
from conan.errors import ConanException
from conan.tools.microsoft import MSBuild, MSBuildDeps, MSBuildToolchain

required_conan_version = ">=2.9"


class RekordBoxSongExporter(ConanFile):
    """
    - Add local recipes with:

    conan remote add local-rbse-repo ./conan --allowed-packages="link/*" --force

    - Install and build with:

    conan build . -of conan/build/debug -pr:a ./conan/profiles/debug --build missing
    or
    conan build . -of conan/build/release -pr:a ./conan/profiles/release --build missing
    """
    name = "RekordBoxSongExporter"
    version = "4.0.0"
    description = "A hack for Rekordbox on Windows x64 to export track information for realtime integration with OBS " \
                  "and Ableton Link"
    package_type = "application"
    settings = "build_type", "os", "arch", "compiler"
    build_policy = "missing"

    _platform = None

    def requirements(self):
        capstone_options = {
            "x86": True,
            "arm": False,
            # "arm64": False, # this should be available, but it isn't
            # "mos65xx": False, # this should be available, but it isn't
            # "wasm": False, # this should be available, but it isn't
            # "bpf": False, # this should be available, but it isn't
            # "riscv": False, # this should be available, but it isn't
            # "sh": False, # this should be available, but it isn't
            # "tricore": False, # this should be available, but it isn't
            "m68k": False,
            "mips": False,
            "ppc": False,
            "sparc": False,
            "sysz": False,
            "xcore": False,
            "tms320c64x": False,
            "m680x": False,
            "evm": False,
        }
        self.requires("capstone/5.0.1", options=capstone_options)
        self.requires("link/3.1.2")

    def validate(self):
        if self.settings.os != "Windows" or self.settings.compiler != "msvc" or self.settings.arch != "x86_64":
            raise ConanException("Only x86_64 Windows with MSVC is supported")
        if self.settings.arch == "x86_64":
            self._platform = "x64"
        else:
            raise ConanException("Unsupported architecture")

    def generate(self):
        ms = MSBuildDeps(self)
        ms.generate()
        tc = MSBuildToolchain(self)
        if self.settings.os == "Linux":
            tc.preprocessor_definitions["LINK_PLATFORM_LINUX"] = 1
        elif self.settings.os == "Macos":
            tc.preprocessor_definitions["LINK_PLATFORM_MACOS"] = 1
        elif self.settings.os == "Windows":
            tc.preprocessor_definitions["LINK_PLATFORM_WINDOWS"] = 1
        tc.generate()

    def build(self):
        msbuild = MSBuild(self)
        # hacky way to add parameters to the build command
        msbuild.platform = f"{self._platform} /p:ForceImportBeforeCppProps={self.build_folder}/conantoolchain.props " \
                           f"/p:ForceImportAfterCppDefaultProps={self.build_folder}/conandeps.props"
        msbuild.build(f"{self.source_folder}/RekordBoxSongExporter.sln")
