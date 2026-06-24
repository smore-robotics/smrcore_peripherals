import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class RCorePeripheralsConan(ConanFile):
    name = "rcore-peripherals"
    package_type = "library"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "build_tests": [True, False],
    }
    default_options = {
        "shared": True,
        "build_tests": True,
    }

    def set_version(self):
        self.version = os.environ.get("SMRCORE_PERIPHERALS_VERSION") or "0.0.0"
    def requirements(self):
        self.requires("nlohmann_json/3.12.0")

    exports_sources = (
        "CMakeLists.txt",
        "include/*",
        "src/*",
        "app/*",
        "config/*",
        "unittests/*",
        "python/*",
        "cmake/*",
    )

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = self.options.build_tests
        tc.variables["SMR_PERIPHERAL_BUILD_PYTHON"] = False
        tc.variables["SMR_PERIPHERAL_WITH_SDK"] = False
        tc.variables["SMRCORE_PERIPHERALS_VERSION"] = self.version
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["smrcore_peripherals"]
