import unreal
for mp in ["/Game/Office/OfficeMats/MI_Concrete01.MI_Concrete01"]:
    mi=unreal.load_asset(mp); par=mi.get_editor_property("parent")
    print("MI_Concrete01 parent=", par.get_name() if par else None)
    for p in mi.get_editor_property("vector_parameter_values"):
        v=p.parameter_value; print("  vec",p.parameter_info.name,"=(%.2f,%.2f,%.2f)"%(v.r,v.g,v.b))
    for p in mi.get_editor_property("scalar_parameter_values"):
        print("  scalar",p.parameter_info.name,"=",p.parameter_value)
# grass master
g=unreal.load_asset("/Game/office_StarterContent/Materials/M_Ground_Grass")
print("\nM_Ground_Grass class=", g.get_class().get_name())
# tree size
for tp in ["/PCG/SampleContent/SimpleForest/Meshes/PCG_Tree_01.PCG_Tree_01",
           "/PCG/SampleContent/SimpleForest/Meshes/PCG_Tree_02.PCG_Tree_02"]:
    t=unreal.load_asset(tp); e=t.get_bounds().box_extent; o=t.get_bounds().origin
    print("%s extent=(%.0f,%.0f,%.0f) originZ=%.0f"%(t.get_name(),e.x,e.y,e.z,o.z))
