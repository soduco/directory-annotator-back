from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import copy
import os.path

class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    default_options = {
        "pylene/*:fPIC": True
    }

    def generate(self):
        # Copy dynamic libraries
        for dep in self.dependencies.values():
            print(dep.ref.name, dep.cpp_info.libs, dep.cpp_info.libdirs)
            if len(dep.cpp_info.libdirs) > 0 and "shared" in dep.options and dep.options.shared == True:
                copy(self, "*.dylib", dep.cpp_info.libdirs[0], os.path.join(self.build_folder, "lib"))
                copy(self, "*.dll", dep.cpp_info.libdirs[0], os.path.join(self.build_folder, "lib"))
                copy(self, "*.so*", dep.cpp_info.libdirs[0], os.path.join(self.build_folder, "lib"))

    def requirements(self):
        self.requires("fmt/[^9.0]", override=True)
        self.requires("libpng/[~1.6]", override=True)
        self.requires("spdlog/[>=1.10]")
        self.requires("pybind11/2.9.2")
        self.requires("pylene/head@lrde/unstable")
        self.requires("pylene-numpy/head@lrde/stable")
        self.requires("blend2d/0.0.18")
        self.requires("nlohmann_json/[>3.0]")
        self.requires("cli11/[>=2.0]")
        self.requires("poppler/21.07.0@lrde")
        self.requires("uwebsockets/[~20]")
        self.requires("cpprestsdk/2.10.18")
        self.requires("boost/1.81.0", override=True)

        self.test_requires("gtest/[^1.12]")



    def build(self):
        # Either using some of the Conan built-in helpers
        cmake = CMake(self)
        cmake.configure()  # equivalent to self.run("cmake . <other args>")
        cmake.build() # equivalent to self.run("cmake --build . <other args>")


    def package(self):
        cmake = CMake(self)
        cmake.install()
