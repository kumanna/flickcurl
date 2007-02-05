/*
 * gcc -o example example.c `flickcurl-config --cflags` `flickcurl-config --libs`
 */

#include <stdio.h>
#include <flickcurl.h>

int main() {
  flickcurl *fc;
  flickcurl_photo *photo;
  flickcurl_photo_field field;
  int i;
  
  fc=flickcurl_new();

  /* Set configuration, or more likely read from a config file */
  flickcurl_set_api_key(fc, "...");
  flickcurl_set_shared_secret(fc, "...");
  flickcurl_set_auth_token(fc, "...");

  photo=flickcurl_photos_getInfo(fc, "123456789"); /* photo ID */

  for(field=0; field <= PHOTO_FIELD_LAST; field++) {
    flickcurl_field_value_type datatype=photo->fields[field].type;
    
    if(datatype != VALUE_TYPE_NONE)
      fprintf(stderr, "field %s (%d) with %s value: '%s' / %d\n", 
              flickcurl_get_photo_field_label(field), field,
              flickcurl_get_field_value_type_label(datatype),
              photo->fields[field].string, photo->fields[field].integer);
  }

  for(i=0; i < photo->tags_count; i++) {
    flickcurl_tag* tag=photo->tags[i];
    fprintf(stderr, "%d) %s tag: id %s author %s raw '%s' cooked '%s'\n",
            i, (tag->machine_tag ? "machine" : "regular"),
            tag->id, tag->author, tag->raw, tag->cooked);
  }

  flickcurl_free_photo(photo);

  flickcurl_free(fc);
}