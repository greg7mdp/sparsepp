#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conan import ConanFile, tools
from conan.tools.files import get,copy
import os

class SparseppConan(ConanFile):
    name = "sparsepp"
    version = "1.22"
    description = "A fast, memory efficient hash map for C++"
    
    # Indicates License type of the packaged library
    license = "https://github.com/greg7mdp/sparsepp/blob/master/LICENSE"
    
    # Packages the license for the conanfile.py
    exports = ["LICENSE"]
    
    # Custom attributes for Bincrafters recipe conventions
    source_subfolder = "source_subfolder"
    
    package_type = "header-library"
    
    def source(self):
        source_url = "https://github.com/greg7mdp/sparsepp"
        get(self, "{0}/archive/{1}.tar.gz".format(source_url, self.version))
        extracted_dir = self.name + "-" + self.version

        #Rename to "source_folder" is a convention to simplify later steps
        os.rename(extracted_dir, self.source_subfolder)


    def package(self):
        include_folder = os.path.join(self.source_subfolder, "sparsepp")
        copy(self, "*", include_folder, self.package_folder)

    def package_id(self):
        self.info.clear()
    