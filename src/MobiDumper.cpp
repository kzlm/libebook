/* 
 * MobiDumper 
 * Utilities for content extraction and
 * markup "normalization" of mobi files
 * 
 * Author:  Domenico Rotiroti
 * License: GPL3 (see COPYING)
 */

#include "MobiDumper.h"
#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <string>

using std::string;
using std::vector;

#define FPOSLEN 10

MobiDumper::~MobiDumper() {
}

void MobiDumper::dumpMetadata() {
    std::map<string, string> meta;
    meta["author"] = book->getAuthor();
    meta["title"] = book->getTitle();
    meta["publisher"] = book->getPublisher();
    meta["cover"] = imgNames[mobi->getCoverIndex()];

    write("info.json", jsonize(meta));
}

string MobiDumper::fixLinks(string src) {
    char fbuf[24];

    // Step 1. fix a[@href]    
    string fmark = "filepos=", href="href=text_";
    size_t fml = fmark.length(), hl = href.length();
    string::size_type pos = src.find(fmark);
    while(pos!=string::npos) {
	src.replace(pos, fml, href);
	src.insert(pos+hl+FPOSLEN,".html");
	pos = src.find(fmark, pos+hl+FPOSLEN);
    }


    // Step 2. fix img[@src]
    for(int i = 0; i < imgNames.size(); ++i) {
	sprintf(fbuf, "recindex=\"%05d\"", i+1);
	string f1 = string(fbuf);
	string r1 = "src=\"";
	r1.append(imgNames[i]);
	r1.append("\"");
	replaceAll(src, f1, r1);
    }

    return src;
}

void MobiDumper::dumpText() {
    char fbuf[24];
    string text = mobi->getText(), 
	    part;

    for( vector<int>::iterator ip = filepos.begin(); ip != filepos.end(); ip++) {
	//split at *ip
	part = fixLinks(text.substr(*ip));
	text = text.substr(0,*ip);
	//write part
	sprintf(fbuf, "text_%010d.html", *ip);
	write(fbuf, part);
    }
    
    write("text.html", fixLinks(text));
}

void MobiDumper::dumpResources() {
	ImageData * id;
	
	for(int i = 1; i <= mobi->imagesCount; ++i) {
	    id = mobi->getImage(i);
	    if(id==NULL) break;
	    
	    write(imgNames[i-1].c_str(), id->data, id->len);
	}
}

void MobiDumper::scanImages() {
	ImageData * id;
	char fname[PATHLEN];
	
	for(int i = 1; i <= mobi->imagesCount; ++i) {
	    id = mobi->getImage(i);
	    if(id==NULL) break;
	    sprintf(fname, "img_%03d%s", i, id->type);
	    imgNames.push_back(string(fname));
	}
}

void MobiDumper::scanLinks() {
    string txt = mobi->getText();
    string fmark = "filepos=";
    size_t fml = fmark.length();
    int val;
    
    // find all filepos marks (avoiding duplicates)
    string::size_type pos = txt.find(fmark);
    while(pos!=string::npos) {
	val = atoi(txt.substr(pos+fml, FPOSLEN).c_str());
	if( std::find(filepos.begin(), filepos.end(), val) == filepos.end())
		filepos.push_back(val);
	pos = txt.find(fmark, pos+fml);
    }
    
    // sort them in reverse order
    std::sort(filepos.begin(), filepos.end(), std::greater<int>());
}