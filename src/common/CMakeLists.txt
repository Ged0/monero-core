set(common_headers)

set(common_private_headers
	file_util.h
	aligned.h)

set(common_sources
	aligned.c)

monero_private_headers(common
  ${common_private_headers})
monero_add_library(common
  ${common_sources}
  ${common_headers}
  ${common_private_headers})
  # DEPENDS generate_translations_header)