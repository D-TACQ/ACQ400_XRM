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
DB += permit_AI.template permit_AI_16CH.substitutions permit_AI_16CH.db
DB += permit_AI.template permit_AI_32CH.substitutions permit_AI_32CH.db
DB += permit_lut.template permit_lut_32R.substitutions permit_lut_32R.db
DB += permit_lut.template permit_lut_16R.substitutions permit_lut_16R.db

# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

