.PHONY: clean

MONGOOSE_VERSION := 7.8
CJSON_VERSION := 1.7.15
#MACHINE_ROOT_DIR := /opt/riscv/rootfs/buildroot/work/staging

all: mongoose cJSON

mongoose:
	wget https://github.com/cesanta/mongoose/archive/refs/tags/${MONGOOSE_VERSION}.tar.gz
	tar xvf ${MONGOOSE_VERSION}.tar.gz
	rm ${MONGOOSE_VERSION}.tar.gz
	mv mongoose-${MONGOOSE_VERSION} mongoose

cJSON:
	wget https://github.com/DaveGamble/cJSON/archive/refs/tags/v${CJSON_VERSION}.tar.gz
	tar xvf v${CJSON_VERSION}.tar.gz
	rm v${CJSON_VERSION}.tar.gz
	mv cJSON-${CJSON_VERSION} cJSON
#	make PREFIX=${MACHINE_DIR}/usr/local/include/cjson DESTDIR=${MACHINE_DIR}/usr/local/lib install 

clean:
	rm -rf mongoose
	rm -rf cJSON