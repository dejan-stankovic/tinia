
SET( tutorial_HEADERS "Tutorial4_Job.hpp" )

IF(Tinia_DESKTOP)
ADD_EXECUTABLE(tutorial4_desktop
    ${tutorial_HEADERS}
    tutorial4_desktop.cpp
)
  TARGET_LINK_LIBRARIES(tutorial4_desktop
	      tinia_model
    tinia_qtcontroller
    tinia_renderlist
    tinia_renderlistgl
    ${QT_LIBRARIES}
    ${QT_QTOPENGL_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARY}
    tinia_model
    tinia_jobcontroller
    ${Boost_LIBRARIES}
    )
ENDIF()


IF(Tinia_SERVER)
  ADD_EXECUTABLE(tutorial4_web
    ${tutorial_HEADERS}
    tutorial4_web.cpp
    )

  TARGET_LINK_LIBRARIES(tutorial4_web
    ${TINIA_LIBRARIES}
    ${GLEW_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${Boost_LIBRARIES} 
    ${LIBXML2_LIBRARIES} 
    tinia_model 
    tinia_modelxml
    tinia_renderlist
    tinia_renderlistgl
    tinia_jobcontroller
    tinia_trell
    )


  INSTALL(TARGETS
    tutorial4_web
    EXPORT TiniaTargets
    RUNTIME
    DESTINATION var/trell/apps/
    COMPONENT mod_trell
    )
ENDIF()
