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

#include "wiki-pagetitle.h"
#include <cstdlib>
#include <fstream>
#include <iostream>


WikiPageTitles* WikiPageTitles::m_pInstance = 0;

WikiPageTitles* WikiPageTitles::Instance()
{
  if(!m_pInstance)
    m_pInstance = new WikiPageTitles;

  return m_pInstance;
}

int32_t WikiPageTitles::find(std::string name)
{
  std::unordered_map<std::string,int32_t>::iterator it;

  it = page_titles.find(name);
  if(it != page_titles.end())
  {
    return (*it).second; 
  }
  else
    return -1;
}

void WikiPageTitles::load(std::string filename)
{
  int32_t titles_index = 0;
  std::string current_title = "";
  std::ifstream titles_file;
  titles_file.open(filename.c_str());
  std::cout << "Loading file " << filename << "\n";
  while(getline(titles_file,current_title))
  {
    std::pair<std::string,int32_t> entry (current_title,titles_index);

    // create an index only if no element with that key was present
    if( page_titles.insert(entry).second ){
      map_indexes.push_back(current_title);
      titles_index++;
    }
  }
  titles_file.close();

  m_pageTitlesSize = page_titles.size();
  std::cout << "Page titles loaded with " << titles_index << " titles"<< "\n"; 
  std::cout << "The bucket has size " << m_pageTitlesSize << " titles"<< "\n"; 
}

uint32_t WikiPageTitles::size(){ return m_pageTitlesSize;}

// this method is intended to pick up a random element and then erase it from the set
std::string WikiPageTitles::pickElement(uint32_t increment)
{
  std::string result = "";
  std::unordered_map<std::string,int32_t>::iterator it = page_titles.begin();
  if(it != page_titles.end()){
    if (increment < m_pageTitlesSize )
      std::advance(it,increment); 
    else if (m_pageTitlesSize != 1)
      std::advance(it,1);
    result = (*it).first;
    page_titles.erase(it);
  }
  return result;
}

// this method is intended to read an element at a specific position
std::string WikiPageTitles::readElement(uint32_t index)
{
  return map_indexes[index % m_pageTitlesSize];
}
