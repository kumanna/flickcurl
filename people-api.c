/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * people-api.c - Flickr flickr.people.* API calls
 *
 * Copyright (C) 2007, David Beckett http://purl.org/net/dajobe/
 * 
 * This file is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#undef HAVE_STDLIB_H
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <flickcurl.h>
#include <flickcurl_internal.h>


/**
 * flickcurl_people_findByEmail:
 * @fc: flickcurl context
 * @email: user email address
 * 
 * Get a user's NSID, given their email address
 *
 * Implements flickr.people.findByEmail (0.8)
 * 
 * Return value: NSID or NULL on failure
 **/
char*
flickcurl_people_findByEmail(flickcurl* fc, const char* email)
{
  return flickcurl_call_get_one_string_field(fc, "find_email", email, 
                                             "flickr.people.findByEmail",
                                             (const xmlChar*)"/rsp/user/@nsid");
}


/**
 * flickcurl_people_findByUsername:
 * @fc: flickcurl context
 * @username: username
 * 
 * Get a user's NSID, given their username address
 *
 * Implements flickr.people.findByUsername (0.8)
 * 
 * Return value: NSID or NULL on failure
 **/
char*
flickcurl_people_findByUsername(flickcurl* fc, const char* username)
{
  return flickcurl_call_get_one_string_field(fc, "username", username, 
                                             "flickr.people.findByUsername",
                                             (const xmlChar*)"/rsp/user/@nsid");
}


/**
 * flickcurl_people_getInfo - 
 * @fc: flickcurl context
 * @user_id: user NSID
 * 
 * Get information about a person
 *
 * Implements flickr.people.getInfo (0.6)
 *
 * NSID can be found by flickcurl_people_findByEmail() or
 * flickcurl_people_findByUsername().
 * 
 * Return value: #flickcurl_person object or NULL on failure
 **/
flickcurl_person*
flickcurl_people_getInfo(flickcurl* fc, const char* user_id)
{
  const char * parameters[6][2];
  int count=0;
  xmlDocPtr doc=NULL;
  xmlXPathContextPtr xpathCtx=NULL; 
  flickcurl_person* person=NULL;
  
  parameters[count][0]  = "user_id";
  parameters[count++][1]= user_id;

  parameters[count][0]  = NULL;

  if(flickcurl_prepare(fc, "flickr.people.getInfo", parameters, count))
    goto tidy;

  doc=flickcurl_invoke(fc);
  if(!doc)
    goto tidy;

  xpathCtx = xmlXPathNewContext(doc);
  if(!xpathCtx) {
    flickcurl_error(fc, "Failed to create XPath context for document");
    fc->failed=1;
    goto tidy;
  }

  person=flickcurl_build_person(fc, xpathCtx, (const xmlChar*)"/rsp/person");

 tidy:
  if(xpathCtx)
    xmlXPathFreeContext(xpathCtx);
  if(fc->failed)
    person=NULL;

  return person;
}


/**
 * flickcurl_people_getPublicGroups:
 * @fc: flickcurl context
 * @user_id: The NSID of the user to fetch groups for.
 * 
 * Returns the list of public groups a user is a member of.
 *
 * Implements flickr.people.getPublicGroups (0.13)
 * 
 * Return value: non-0 on failure
 **/
flickcurl_group**
flickcurl_people_getPublicGroups(flickcurl* fc, const char* user_id)
{
  const char* parameters[8][2];
  int count=0;
  xmlDocPtr doc=NULL;
  xmlXPathContextPtr xpathCtx=NULL; 
  flickcurl_group** groups=NULL;
  
  if(!user_id)
    return 1;

  parameters[count][0]  = "user_id";
  parameters[count++][1]= user_id;

  parameters[count][0]  = NULL;

  if(flickcurl_prepare(fc, "flickr.people.getPublicGroups", parameters, count))
    goto tidy;

  doc=flickcurl_invoke(fc);
  if(!doc)
    goto tidy;


  xpathCtx = xmlXPathNewContext(doc);
  if(!xpathCtx) {
    flickcurl_error(fc, "Failed to create XPath context for document");
    fc->failed=1;
    goto tidy;
  }

  groups=flickcurl_build_groups(fc, xpathCtx,
                                (const xmlChar*)"/rsp/groups/group", NULL);

  tidy:
  if(xpathCtx)
    xmlXPathFreeContext(xpathCtx);

  if(fc->failed)
    groups=NULL;

  return groups;
}


/**
 * flickcurl_people_getPublicPhotos:
 * @fc: flickcurl context
 * @user_id: The NSID of the user who's photos to return.
 * @extras: A comma-delimited list of extra information to fetch for each returned record.
 * @per_page: Number of photos to return per page (default 100, max 500)
 * @page: The page of results to return (default 1)
 * 
 * Get a list of public photos for the given user.
 *
 *  Currently supported extras fields are: license, date_upload,
 *  date_taken, owner_name, icon_server, original_format,
 *  last_update, geo, tags, machine_tags.

 * Implements flickr.people.getPublicPhotos (0.12)
 * 
 * Return value: non-0 on failure
 **/
flickcurl_photo**
flickcurl_people_getPublicPhotos(flickcurl* fc, const char* user_id, 
                                 const char* extras, 
                                 int per_page, int page)
{
  const char* parameters[11][2];
  int count=0;
  xmlDocPtr doc=NULL;
  xmlXPathContextPtr xpathCtx=NULL; 
  flickcurl_photo** photos=NULL;
  char per_page_s[10];
  char page_s[10];
  
  if(!user_id)
    return NULL;

  parameters[count][0]  = "user_id";
  parameters[count++][1]= user_id;
  if(extras) {
    parameters[count][0]  = "extras";
    parameters[count++][1]= extras;
  }
  parameters[count][0]  = "per_page";
  sprintf(per_page_s, "%d", per_page);
  parameters[count++][1]= per_page_s;
  parameters[count][0]  = "page";
  sprintf(page_s, "%d", page);
  parameters[count++][1]= page_s;

  parameters[count][0]  = NULL;

  if(flickcurl_prepare(fc, "flickr.people.getPublicPhotos", parameters, count))
    goto tidy;

  doc=flickcurl_invoke(fc);
  if(!doc)
    goto tidy;


  xpathCtx = xmlXPathNewContext(doc);
  if(!xpathCtx) {
    flickcurl_error(fc, "Failed to create XPath context for document");
    fc->failed=1;
    goto tidy;
  }

  photos=flickcurl_build_photos(fc, xpathCtx,
                                (const xmlChar*)"/rsp/photos/photo", NULL);

  tidy:
  if(xpathCtx)
    xmlXPathFreeContext(xpathCtx);

  if(fc->failed)
    photos=NULL;

  return photos;
}



/*
 * flickr.people.getUploadStatus:
 *
 * Get information for the calling user related to photo uploads.
 */
