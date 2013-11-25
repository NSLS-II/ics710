TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# Create and install (or just install)
# databases, templates, substitutions like this
#DB += dbExample1.db
#DB += dbExample2.db
#DB += dbSubExample.db
#DB += user.substitutions
#DB += userHost.substitutions
#DB += ics710Channel.template
#DB += ics710Channel.substitutions
#DB += ics710Card.template
#DB += ics710Card.substitutions
DB += ics710Channel.db
DB += ics710Card.db
DB += ictQ.db
DB += ictQSpecific.db
DB += dcct.db
DB += dcctBR.db
DB_INSTALLS += $(EPICS_BASE)/db/iocAdminSoft.db
DB_INSTALLS += $(EPICS_BASE)/db/save_restoreStatus.db
#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

