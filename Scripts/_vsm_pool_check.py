import unreal

def getf(cv):
    try:
        return unreal.SystemLibrary.get_console_variable_float_value(cv)
    except Exception as e:
        return "err:" + str(e)[:30]

for cv in ["r.Shadow.Virtual.MaxPhysicalPages",
           "r.Shadow.Virtual.Enable",
           "r.Shadow.Virtual.ResolutionLodBiasLocal",
           "r.Shadow.Virtual.Cache",
           "r.Shadow.Virtual.Cache.StaticSeparate"]:
    print(cv, "=", getf(cv))
