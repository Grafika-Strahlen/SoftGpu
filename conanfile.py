from conan import ConanFile
from conan.tools.microsoft import MSBuildDeps

class SoftGpu(ConanFile):
    settings = "os", "compiler", "arch", "build_type"
    requires = "tauutils/[^1.3.10]"

    def generate(self):
        ms = MSBuildDeps(self)
        ms.generate()
        if self.settings.build_type == "Debug":
            ms.configuration = "TestSetup"
            ms.generate()

    def layout(self):
        self.folders.generators = "libs/conan"

