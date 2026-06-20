import unreal
names = [n for n in dir(unreal) if "ModifyBone" in n or "AnimGraphNode" in n or n=="AnimBlueprintFactory" or "BoneControl" in n]
print("relevant unreal classes:", names[:30])
print("has AnimBlueprintFactory:", hasattr(unreal,"AnimBlueprintFactory"))
print("has AnimGraphNode_ModifyBone:", hasattr(unreal,"AnimGraphNode_ModifyBone"))
