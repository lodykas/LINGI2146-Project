CONTIKI_PROJECT = project
all: $(CONTIKI_PROJECT)

#UIP_CONF_IPV6=1

CONTIKI = ../../contiki
include $(CONTIKI)/Makefile.include
