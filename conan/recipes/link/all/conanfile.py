from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.files import copy, symlinks
from conan.tools.scm import Git

required_conan_version = ">=2.9"


class Link(ConanFile):
    name = "link"
    version = "3.1.2"
    _digest = "41d9aa111f702e78b6fbaee9d3e06dda1db6420d"
    url = "https://github.com/Ableton/Link"
    homepage = "https://ableton.github.io/link/"
    description = "Ableton Link"
    settings = "build_type", "os", "arch", "compiler"
    build_policy = "missing"

    def source(self):
        git = Git(self)
        git.fetch_commit(self.url, self._digest)
        git.run("submodule update --init --recursive")

    def build_requirements(self):
        self.tool_requires("cmake/3.31.3")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, pattern="*", src=f"{self.source_folder}/include/ableton", dst=f"{self.package_folder}/include/ableton")
        copy(self, pattern="*", src=f"{self.source_folder}/third_party/catch", dst=f"{self.package_folder}/include/catch")
        copy(self, pattern="*", src=f"{self.source_folder}/modules/asio-standalone/asio/include", dst=f"{self.package_folder}/include")
        copy(self, pattern="*", src=f"{self.build_folder}/{self.settings.build_type}", dst=f"{self.package_folder}/lib", keep_path=False)
        copy(self, pattern="*", src=f"{self.build_folder}/bin", dst=f"{self.package_folder}/bin", keep_path=False)
        symlinks.absolute_to_relative_symlinks(self, self.package_folder)

    def package_info(self):
        self.cpp_info.libs = ["abl_link"]
