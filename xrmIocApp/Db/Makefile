TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Install databases, templates & substitutions like this
DB += circle.db
DB += dbExample1.db
DB += dbExample2.db
DB += xrmIocVersion.db
DB += dbSubExample.db
DB += user.substitutions

DB += RecCaster.substitutions
DB += ch.template ch32.substitutions ch32.db
DB += ch.template ch16.substitutions ch16.db
DB += ch.template ch8.substitutions ch8.db

# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

