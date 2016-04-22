bin_SCRIPTS=mdbshm_reader.py
pkgdata_datadir=${prefix}/
pkgdata_data_DATA=do_dump.sh do_upload.sh tair.sh set_shm.sh stat.sh check_version.sh server.js.bk run_rsync_config_server.sh
EXTRA_DIST=do_dump.sh do_upload.sh tair.sh set_shm.sh stat.sh check_version.sh server.js.bk run_rsync_config_server.sh

install-data-hook:
	for f in ${EXTRA_DIST}; \
	do \
	  chmod +x ${DESTDIR}/${prefix}/$$f ;\
	done
