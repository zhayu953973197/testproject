#!/usr/bin/env python
# coding=utf-8
# -----------------------------------------------------------------------------------------
#Purpose:
#Copyright CompanyNameMagicTag 2018-2019. All rights reserved.
#Author: CompanyName
#------------------------------------------------------------------------------------------

import sys
import os
import re
import operator
import shutil

#add .init .hot .isr .exit etc. exp:".kernel.rodata"
sections = ['.text','.data', '.rodata', '.bss', 'COMMON']
pre_sections = ['\S*\.text','\.data', '\.rodata', '\.bss', 'COMMON']
re_sections = [re.compile(x) for x in pre_sections]

result = {}

def init_dict(lib):
    result.setdefault(lib, {})
    for section in sections:
        result[lib].setdefault(section, 0)

def work():
    starting, last_section = False, ''
    re_lib = re.compile(r'^0x02\S*\s*(0x\w+)\s\S\:/([^\s)(]+\.[ao])\S*')
    re_section_lib = [re.compile(r'^%s\.*\S*\s*0x02\S*\s*(0x\w+)\s\S\:/([^\s)(]+\.[ao])\S*'%x) for x in pre_sections]
    with open(sys.argv[1]) as fp:
        for line in fp:
            line = line.strip()
            length = len(line.split())
            if not starting:
                if 'Discarded input sections' in line:
                    starting = True
                continue

            do_continue = False
     
            
            if length == 4:
                for i, slp in enumerate(re_section_lib):
                    match = slp.match(line)
                    if match:
                        size, lib = match.groups()
                        section, do_continue = sections[i], True
                        if lib not in result:
                            init_dict(lib)
                        result[lib][section] = result[lib][section] + eval(size)
                        continue

            if do_continue:
                continue

            if length == 1:
                for i, sp in enumerate(re_sections):
                    if sp.match(line):
                        last_section, do_continue = sections[i], True
                        break
                if not do_continue:
                    last_section = None

            if do_continue:
                continue

            if length == 3:
                match = re_lib.match(line)
                if match:

                    size, lib = match.groups()
                    if lib not in result:
                        init_dict(lib)
                    if last_section:
                        result[lib][last_section] = result[lib][last_section] + eval(size)
                    continue

            if '__bss_start = ABSOLUTE (.)' in line:
                return

def output():
    holy_total = 0
    print ('parse   %s'%sys.argv[1])
    for lib, detail in result.items():
        print (lib)

        total_size = 0
        for section, size in result[lib].items():
            total_size = total_size + size

        holy_total = holy_total + total_size

        print ('%-8s %6.1fk'%('total:', round(total_size / 1024.0, 1)))
        for section in sections:
            size = result[lib][section]

        if total_size > 0:
            for section, size in result[lib].items():
                print ('%-8s %6.1fk\t%2.1f%%'%('%s:'%section, round(size / 1024.0, 1), round(float(size) / total_size, 1)))
                continue

    print (round(holy_total / 1024.0 , 1), 'K')
	
if __name__ == '__main__':
    work()
    output()
