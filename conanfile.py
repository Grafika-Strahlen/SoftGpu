from conan import ConanFile
from conan.tools.microsoft import MSBuildDeps
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps

class SoftGpu(ConanFile):
    settings = "os", "compiler", "arch", "build_type"
    requires = "tauutils/[^1.4.0]", "glm/[^1.0.1]", "vulkan-memory-allocator/[^3.0.1]"

    def configure(self):
        self.options["tauutils"].shared = False

    def generate(self):
        ms = MSBuildDeps(self)
        ms.generate()
        if self.settings.build_type == "Debug":
            ms.configuration = "TestSetup"
            ms.generate()
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def layout(self):
        cmake_layout(self)
        # self.folders.generators = "libs/conan"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

