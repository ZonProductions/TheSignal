"""Check ALL components and attached actors on F5 elevator wall."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label() == 'F5_SM_ElevatorWall2':
        # All components
        comps = a.get_components_by_class(unreal.ActorComponent)
        print(f"Components on F5_SM_ElevatorWall2: {len(comps)}")
        for c in comps:
            cn = c.get_class().get_name()
            print(f"  {cn}: {c.get_name()}")
            if hasattr(c, 'get_num_materials'):
                for i in range(c.get_num_materials()):
                    mat = c.get_material(i)
                    print(f"    mat[{i}]: {mat.get_name() if mat else 'None'}")

        # Attached actors
        att = a.get_attached_actors()
        print(f"\nAttached actors: {len(att)}")
        for x in att:
            print(f"  {x.get_actor_label()} ({x.get_class().get_name()})")

        # Check mesh element count at each LOD
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        sm = smc.get_editor_property("static_mesh") if smc else None
        if sm:
            for lod in range(sm.get_num_lods()):
                sections = sm.get_num_sections(lod)
                print(f"\nMesh LOD {lod}: {sections} sections")

            # Material slot names
            slot_names = sm.get_material_slot_names()
            print(f"\nMaterial slot names: {[str(n) for n in slot_names]}")

            # Mesh bounds
            bounds = sm.get_bounds()
            print(f"Mesh bounds - Origin: ({bounds.origin.x:.1f}, {bounds.origin.y:.1f}, {bounds.origin.z:.1f})")
            print(f"Mesh bounds - Extent: ({bounds.box_extent.x:.1f}, {bounds.box_extent.y:.1f}, {bounds.box_extent.z:.1f})")

            # Section info
            for s in range(sm.get_num_sections(0)):
                try:
                    mat_idx = sm.get_section_from_static_mesh(s, 0)
                except:
                    pass

        break
