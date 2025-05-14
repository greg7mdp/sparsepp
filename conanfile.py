#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conan import ConanFile, tools
from conan.tools.files import get,copy
import os

class SparseppConan(ConanFile):
    name = "sparsepp"
    version = "1.22"
    description = "A fast, memory efficient hash map for C++"
    license = "https://github.com/greg7mdp/sparsepp/blob/master/LICENSE"

    exports_sources = "sparsepp/*"
    no_copy_source = True
    exports = ["LICENSE"]
    package_type = "header-library"

    def package(self):
        copy(self, "*",
             self.source_folder,
             os.path.join(self.package_folder, "include"))

    def package_id(self):
        self.info.clear()

    def package_info(self):
        self.cpp_info.includedirs = [os.path.join(self.package_folder, "include")]