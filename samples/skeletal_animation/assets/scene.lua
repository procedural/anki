-- Generated by: /home/godlike/src/anki/buildd/bin/gltf_importer droid.gltf /home/godlike/src/anki/samples/skeletal_animation/assets/ -rpath assets -texrpath assets
local scene = getSceneGraph()
local events = getEventManager()

node = scene:newModelNode("droid.001")
node:getSceneNodeBase():getModelComponent():loadModelResource("assets/Mesh_Robot.001.ankimdl")
node:getSceneNodeBase():getSkinComponent():loadSkeletonResource("assets/Armature.002.ankiskel")
trf = Transform.new()
trf:setOrigin(Vec4.new(0.000000, 0.000000, 0.000000, 0))
rot = Mat3x4.new()
rot:setAll(1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000)
trf:setRotation(rot)
trf:setScale(1.000000)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)

node = scene:newGlobalIlluminationProbeNode("Cube")
comp = node:getSceneNodeBase():getGlobalIlluminationProbeComponent()
comp:setBoxVolumeSize(Vec3.new(19.286558, 19.286558, 19.286558))
trf = Transform.new()
trf:setOrigin(Vec4.new(0.000000, 11.057286, 0.000000, 0))
rot = Mat3x4.new()
rot:setAll(1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000)
trf:setRotation(rot)
trf:setScale(1.000000)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)

node = scene:newReflectionProbeNode("Cube.001")
comp = node:getSceneNodeBase():getReflectionProbeComponent()
comp:setBoxVolumeSize(Vec3.new(18.543777, 18.543777, 18.543777))
trf = Transform.new()
trf:setOrigin(Vec4.new(0.000000, 11.057286, 0.000000, 0))
rot = Mat3x4.new()
rot:setAll(1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000)
trf:setRotation(rot)
trf:setScale(1.000000)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)

node = scene:newModelNode("room")
node:getSceneNodeBase():getModelComponent():loadModelResource("assets/room_room.ankimdl")
trf = Transform.new()
trf:setOrigin(Vec4.new(0.000000, 11.142166, 0.000000, 0))
rot = Mat3x4.new()
rot:setAll(1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000)
trf:setRotation(rot)
trf:setScale(9.000000)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)

node = scene:newPointLightNode("Lamp_Orientation")
lcomp = node:getSceneNodeBase():getLightComponent()
lcomp:setDiffuseColor(Vec4.new(100.000000, 100.000000, 100.000000, 1))
lcomp:setShadowEnabled(1)
lcomp:setRadius(100.000000)
trf = Transform.new()
trf:setOrigin(Vec4.new(4.076245, 5.903862, -1.005454, 0))
rot = Mat3x4.new()
rot:setAll(-0.290865, -0.771101, 0.566393, 0.000000, -0.055189, 0.604525, 0.794672, 0.000000, -0.955171, 0.199883, -0.218391, 0.000000)
trf:setRotation(rot)
trf:setScale(1.000000)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)

node = scene:newPerspectiveCameraNode("Camera_Orientation")
scene:setActiveCameraNode(node:getSceneNodeBase())
frustumc = node:getSceneNodeBase():getFrustumComponent()
frustumc:setPerspective(0.100000, 100.000000, getMainRenderer():getAspectRatio() * 0.750416, 0.750416)
frustumc:setShadowCascadesDistancePower(1.5)
frustumc:setEffectiveShadowDistance(100.000000)
trf = Transform.new()
trf:setOrigin(Vec4.new(5.526846, 8.527484, -6.015655, 0))
rot = Mat3x4.new()
rot:setAll(-0.712312, -0.312519, 0.628445, 0.000000, 0.000000, 0.895396, 0.445271, 0.000000, -0.701863, 0.317172, -0.637801, 0.000000)
trf:setRotation(rot)
trf:setScale(1.000000)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)

node = scene:newModelNode("room.001")
node:getSceneNodeBase():getModelComponent():loadModelResource("assets/room.001_room.red.ankimdl")
trf = Transform.new()
trf:setOrigin(Vec4.new(0.000000, 11.142166, 0.000000, 0))
rot = Mat3x4.new()
rot:setAll(1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000)
trf:setRotation(rot)
trf:setScale(9.000000)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)

node = scene:newModelNode("room.002")
node:getSceneNodeBase():getModelComponent():loadModelResource("assets/room.002_room.green.ankimdl")
trf = Transform.new()
trf:setOrigin(Vec4.new(0.000000, 11.142166, 0.000000, 0))
rot = Mat3x4.new()
rot:setAll(1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000)
trf:setRotation(rot)
trf:setScale(9.000000)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)

node = scene:newModelNode("room.003")
node:getSceneNodeBase():getModelComponent():loadModelResource("assets/room.003_room.blue.ankimdl")
trf = Transform.new()
trf:setOrigin(Vec4.new(0.000000, 11.142166, 0.000000, 0))
rot = Mat3x4.new()
rot:setAll(1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000)
trf:setRotation(rot)
trf:setScale(9.000000)
node:getSceneNodeBase():getMoveComponent():setLocalTransform(trf)
