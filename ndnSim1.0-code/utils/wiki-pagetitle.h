/*
Copyright (C) 2016, the University of Luxembourg
Salvatore Signorello <salvatore.signorello@uni.lu>

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef WIKI_PAGETITLE_H_
#define WIKI_PAGETITLE_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <stdint.h>


class WikiPageTitles
{
public:
  static WikiPageTitles* Instance();
  int32_t find(std::string name);
  void load(std::string filename);
  uint32_t size();
  std::string pickElement(uint32_t increment);
  std::string readElement(uint32_t index);

private:
  WikiPageTitles (){};
  WikiPageTitles(WikiPageTitles const&);
  WikiPageTitles&  operator=(WikiPageTitles const&){};
  static WikiPageTitles* m_pInstance;
  uint32_t m_pageTitlesSize;
  std::unordered_map<std::string,int32_t> page_titles;
  std::vector<std::string> map_indexes;
};

#endif
