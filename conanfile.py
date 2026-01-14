from conan import ConanFile

class MultiVNC(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        if self.settings.os in ["Windows"]:
            self.requires("zlib/1.3.1")

        if self.settings.os in ["Windows", "Macos"]:
            self.requires("libjpeg-turbo/3.1.2")
            self.requires("openssl/3.5.4")
            self.requires("libssh2/1.11.1", options = {
                "with_zlib": False
            })
            # only listing libtiff here so it uses libjpeg-turbo, it is actually a dep of wxwidgets
            self.requires("libtiff/[>=4]", options = {
                "jpeg":  "libjpeg-turbo"
            })
            self.requires("wxwidgets/3.2.8", options = {
                "jpeg":  "libjpeg-turbo"
            })

