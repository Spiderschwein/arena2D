set(ARENA_ADDITIONAL_SOURCES
	# add your additional .cpp/.h files here, e.g.
	# level/my_custom_level.cpp	
	# level/my_custom_level.h
)

set(ARENA_SOURCES
# main application
	arena/main.cpp
	arena/Arena.cpp
	arena/Arena_cmd.cpp
	arena/Arena_py.cpp
	arena/Arena_update.cpp
	arena/Arena_render.cpp
	arena/Arena_processEvents.cpp
	arena/Arena.h
	arena/Command.cpp
	arena/Command.h
	arena/CommandRegister.h
	arena/Console.cpp
	arena/Console.h
	arena/ConsoleParameters.cpp
	arena/ConsoleParameters.h
	arena/StatsDisplay.cpp
	arena/StatsDisplay.h
	arena/CSVWriter.cpp
	arena/CSVWriter.h
	arena/PhysicsWorld.cpp
	arena/PhysicsWorld.h
	level/Level.cpp
	level/Level.h
	level/LevelFactory.h
	level/LevelFactory.cpp
	level/LevelEmpty.cpp
	level/LevelEmpty.h
	level/LevelRandom.cpp
	level/LevelRandom.h
	level/LevelSVG.cpp
	level/LevelSVG.h
	level/Wanderer.cpp
	level/Wanderer.h
	level/SVGFile.h
	level/SVGFile.cpp
	arena/Environment.cpp
	arena/Environment.h
	arena/Robot.h
	arena/Robot.cpp
	arena/LidarCast.h
	arena/LidarCast.cpp
	arena/RectSpawn.cpp
	arena/RectSpawn.h

# engine
	engine/shader/Color2DShader.cpp
	engine/shader/Color2DShader.h
	engine/shader/Colorplex2DShader.cpp
	engine/shader/Colorplex2DShader.h
	engine/shader/ParticleShader.cpp
	engine/shader/ParticleShader.h
	engine/shader/SpriteShader.cpp
	engine/shader/SpriteShader.h
	engine/shader/TextShader.cpp
	engine/shader/TextShader.h
	engine/shader/zShaderProgram.cpp
	engine/shader/zShaderProgram.h
	engine/Timer.h
	engine/Timer.cpp
	engine/Camera.cpp
	engine/Camera.h
	engine/f_math.c
	engine/f_math.h
	engine/GamePadButtonCodes.h
	engine/GlobalSettings.cpp
	engine/GlobalSettings.h
	engine/hashTable.c
	engine/hashTable.h
	engine/list.h
	engine/list.c
	engine/ParticleEmitter.cpp
	engine/ParticleEmitter.h
	engine/Quadrangle.h
	engine/Renderer.cpp
	engine/Renderer.h
	engine/SettingsKeys.cpp
	engine/zColor.cpp
	engine/zColor.h
	engine/zFont.cpp
	engine/zFont.h
	engine/zFramework.cpp
	engine/zFramework.h
	engine/zGlyphMap.cpp
	engine/zGlyphMap.h
	engine/zGraphicObject.cpp
	engine/zGraphicObject.h
	engine/zLogfile.cpp
	engine/zLogfile.h
	engine/zMatrix4x4.cpp
	engine/zMatrix4x4.h
	engine/zRect.h
	engine/zRect.cpp
	engine/zSingleton.h
	engine/zStringTools.cpp
	engine/zStringTools.h
	engine/zTextView.cpp
	engine/zTextView.h
	engine/zVector2d.cpp
	engine/zVector2d.h
	engine/zVector3d.cpp
	engine/zVector3d.h
	engine/zVector4d.h

#settings
	settings/SettingsStructs.h
	settings/SettingsDefault.cpp

# additional libraries
	engine/glew/glew.c
	engine/glew/glew.h
	engine/glew/eglew.h
	engine/glew/glxew.h
	engine/glew/wglew.h
)
