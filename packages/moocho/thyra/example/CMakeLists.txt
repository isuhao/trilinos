ASSERT_DEFINED(${PACKAGE_NAME}_ENABLE_Stratimikos)
IF (${PACKAGE_NAME}_ENABLE_Stratimikos)

  ASSERT_DEFINED(${PACKAGE_NAME}_ENABLE_EpetraExt)
  IF (${PACKAGE_NAME}_ENABLE_EpetraExt)
    ADD_SUBDIRECTORY(NLPThyraEpetraModelEval4DOpt)
    ADD_SUBDIRECTORY(NLPThyraEpetraAdvDiffReactOpt)
    ADD_SUBDIRECTORY(DiagonalQuadraticResponseOnlyOpt)
  ENDIF()

  ASSERT_DEFINED(${PACKAGE_NAME}_ENABLE_Rythmos)
  IF (${PACKAGE_NAME}_ENABLE_Rythmos)
    ADD_SUBDIRECTORY(DiagonalTransientInversion)
  ENDIF()

ENDIF()
