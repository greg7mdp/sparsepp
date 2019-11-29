#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, tools
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
    
    def source(self):
        source_url = "https://github.com/greg7mdp/sparsepp"
        tools.get("{0}/archive/{1}.tar.gz".format(source_url, self.version))
        extracted_dir = self.name + "-" + self.version

        #Rename to "source_folder" is a convention to simplify later steps
        os.rename(extracted_dir, self.source_subfolder)


    def package(self):
        include_folder = os.path.join(self.source_subfolder, "sparsepp")
        self.copy(pattern="LICENSE")
        self.copy(pattern="*", dst="include/sparsepp", src=include_folder)

    def package_id(self):
        self.info.header_only()
