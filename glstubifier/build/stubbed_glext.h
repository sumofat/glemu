GL_API GLvoid glCopyTextureLevelsAPPLE (GLuint destinationTexture ,GLuint sourceTexture ,GLint sourceBaseLevel ,GLsizei sourceLevelCount )
{
	return;
}

GL_API GLvoid glRenderbufferStorageMultisampleAPPLE (GLenum target ,GLsizei samples ,GLenum internalformat ,GLsizei width ,GLsizei height )
{
	return;
}

GL_API GLvoid glResolveMultisampleFramebufferAPPLE (void )
{
	return;
}

GL_API GLsync glFenceSyncAPPLE (GLenum condition ,GLbitfield flags )
{
	return;
}

GL_API GLboolean glIsSyncAPPLE (GLsync sync )
{
	return false;
}

GL_API void glDeleteSyncAPPLE (GLsync sync )
{
	return;
}

GL_API GLenum glClientWaitSyncAPPLE (GLsync sync ,GLbitfield flags ,GLuint64 timeout )
{
	return 0;
}

GL_API void glWaitSyncAPPLE (GLsync sync ,GLbitfield flags ,GLuint64 timeout )
{
	return;
}

GL_API void glGetInteger64vAPPLE (GLenum pname ,GLint64 *params )
{
	return;
}

GL_API void glGetSyncivAPPLE (GLsync sync ,GLenum pname ,GLsizei bufSize ,GLsizei *length ,GLint *values )
{
	return;
}

GL_API GLvoid glLabelObjectEXT (GLenum type ,GLuint object ,GLsizei length ,const GLchar *label )
{
	return;
}

GL_API GLvoid glGetObjectLabelEXT (GLenum type ,GLuint object ,GLsizei bufSize ,GLsizei *length ,GLchar *label )
{
	return;
}

GL_API GLvoid glInsertEventMarkerEXT (GLsizei length ,const GLchar *marker )
{
	return;
}

GL_API GLvoid glPushGroupMarkerEXT (GLsizei length ,const GLchar *marker )
{
	return;
}

GL_API GLvoid glPopGroupMarkerEXT (void )
{
	return;
}

GL_API GLvoid GL_APIENTRY glDiscardFramebufferEXT (GLenum target ,GLsizei numAttachments ,const GLenum *attachments )
{
	return;
}

GL_API GLvoid glDrawArraysInstancedEXT (GLenum mode ,GLint first ,GLsizei count ,GLsizei instanceCount )
{
	return;
}

GL_API GLvoid glDrawElementsInstancedEXT (GLenum mode ,GLsizei count ,GLenum type ,const GLvoid *indices ,GLsizei instanceCount )
{
	return;
}

GL_API GLvoid glVertexAttribDivisorEXT (GLuint index ,GLuint divisor )
{
	return;
}

GL_API GLvoid *glMapBufferRangeEXT (GLenum target ,GLintptr offset ,GLsizeiptr length ,GLbitfield access )
{
	return;
}

GL_API GLvoid glFlushMappedBufferRangeEXT (GLenum target ,GLintptr offset ,GLsizeiptr length )
{
	return;
}

GL_API GLvoid glGenQueriesEXT (GLsizei n ,GLuint *ids )
{
	return;
}

GL_API GLvoid glDeleteQueriesEXT (GLsizei n ,const GLuint *ids )
{
	return;
}

GL_API GLboolean glIsQueryEXT (GLuint id )
{
	return false;
}

GL_API GLvoid glBeginQueryEXT (GLenum target ,GLuint id )
{
	return;
}

GL_API GLvoid glEndQueryEXT (GLenum target )
{
	return;
}

GL_API GLvoid glGetQueryivEXT (GLenum target ,GLenum pname ,GLint *params )
{
	return;
}

GL_API GLvoid glGetQueryObjectuivEXT (GLuint id ,GLenum pname ,GLuint *params )
{
	return;
}

GL_API GLvoid glUseProgramStagesEXT (GLuint pipeline ,GLbitfield stages ,GLuint program )
{
	return;
}

GL_API GLvoid glActiveShaderProgramEXT (GLuint pipeline ,GLuint program )
{
	return;
}

GL_API GLuint glCreateShaderProgramvEXT (GLenum type ,GLsizei count ,const GLchar *const *strings )
{
	return 0;
}

GL_API GLvoid glBindProgramPipelineEXT (GLuint pipeline )
{
	return;
}

GL_API GLvoid glDeleteProgramPipelinesEXT (GLsizei n ,const GLuint *pipelines )
{
	return;
}

GL_API GLvoid glGenProgramPipelinesEXT (GLsizei n ,GLuint *pipelines )
{
	return;
}

GL_API GLboolean glIsProgramPipelineEXT (GLuint pipeline )
{
	return false;
}

GL_API GLvoid glGetProgramPipelineivEXT (GLuint pipeline ,GLenum pname ,GLint *params )
{
	return;
}

GL_API GLvoid glValidateProgramPipelineEXT (GLuint pipeline )
{
	return;
}

GL_API GLvoid glGetProgramPipelineInfoLogEXT (GLuint pipeline ,GLsizei bufSize ,GLsizei *length ,GLchar *infoLog )
{
	return;
}

GL_API GLvoid glProgramUniform1iEXT (GLuint program ,GLint location ,GLint x )
{
	return;
}

GL_API GLvoid glProgramUniform2iEXT (GLuint program ,GLint location ,GLint x ,GLint y )
{
	return;
}

GL_API GLvoid glProgramUniform3iEXT (GLuint program ,GLint location ,GLint x ,GLint y ,GLint z )
{
	return;
}

GL_API GLvoid glProgramUniform4iEXT (GLuint program ,GLint location ,GLint x ,GLint y ,GLint z ,GLint w )
{
	return;
}

GL_API GLvoid glProgramUniform1fEXT (GLuint program ,GLint location ,GLfloat x )
{
	return;
}

GL_API GLvoid glProgramUniform2fEXT (GLuint program ,GLint location ,GLfloat x ,GLfloat y )
{
	return;
}

GL_API GLvoid glProgramUniform3fEXT (GLuint program ,GLint location ,GLfloat x ,GLfloat y ,GLfloat z )
{
	return;
}

GL_API GLvoid glProgramUniform4fEXT (GLuint program ,GLint location ,GLfloat x ,GLfloat y ,GLfloat z ,GLfloat w )
{
	return;
}

GL_API GLvoid glProgramUniform1ivEXT (GLuint program ,GLint location ,GLsizei count ,const GLint *value )
{
	return;
}

GL_API GLvoid glProgramUniform2ivEXT (GLuint program ,GLint location ,GLsizei count ,const GLint *value )
{
	return;
}

GL_API GLvoid glProgramUniform3ivEXT (GLuint program ,GLint location ,GLsizei count ,const GLint *value )
{
	return;
}

GL_API GLvoid glProgramUniform4ivEXT (GLuint program ,GLint location ,GLsizei count ,const GLint *value )
{
	return;
}

GL_API GLvoid glProgramUniform1fvEXT (GLuint program ,GLint location ,GLsizei count ,const GLfloat *value )
{
	return;
}

GL_API GLvoid glProgramUniform2fvEXT (GLuint program ,GLint location ,GLsizei count ,const GLfloat *value )
{
	return;
}

GL_API GLvoid glProgramUniform3fvEXT (GLuint program ,GLint location ,GLsizei count ,const GLfloat *value )
{
	return;
}

GL_API GLvoid glProgramUniform4fvEXT (GLuint program ,GLint location ,GLsizei count ,const GLfloat *value )
{
	return;
}

GL_API GLvoid glProgramUniformMatrix2fvEXT (GLuint program ,GLint location ,GLsizei count ,GLboolean transpose ,const GLfloat *value )
{
	return;
}

GL_API GLvoid glProgramUniformMatrix3fvEXT (GLuint program ,GLint location ,GLsizei count ,GLboolean transpose ,const GLfloat *value )
{
	return;
}

GL_API GLvoid glProgramUniformMatrix4fvEXT (GLuint program ,GLint location ,GLsizei count ,GLboolean transpose ,const GLfloat *value )
{
	return;
}

GL_API GLvoid glTexStorage2DEXT (GLenum target ,GLsizei levels ,GLenum internalformat ,GLsizei width ,GLsizei height )
{
	return;
}

GL_API GLvoid GL_APIENTRY glGetBufferPointervOES (GLenum target ,GLenum pname ,GLvoid **params )
{
	return;
}

GL_API GLvoid *GL_APIENTRY glMapBufferOES (GLenum target ,GLenum access )
{
	return;
}

GL_API GLboolean GL_APIENTRY glUnmapBufferOES (GLenum target )
{
	return false;
}

GL_API GLvoid glBindVertexArrayOES (GLuint array )
{
	return;
}

GL_API GLvoid glDeleteVertexArraysOES (GLsizei n ,const GLuint *arrays )
{
	return;
}

GL_API GLvoid glGenVertexArraysOES (GLsizei n ,GLuint *arrays )
{
	return;
}

GL_API GLboolean glIsVertexArrayOES (GLuint array )
{
	return false;
}

