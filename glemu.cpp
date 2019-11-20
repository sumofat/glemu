//NOTE(Ray):GLEMU implementation file is seperated for reading convience only

#ifdef YOYOIMPL

namespace OpenGLEmu
{
    OpenGLEmuState ogs;
    
    void VerifyCommandBuffer(GLEMUBufferState state)
    {
        ogle_verify_com_buf(&ogs,state);
    }
    
    VertexDescriptor CreateDefaultVertexDescriptor()
    {
        return ogle_create_default_vert_desc();
    }
    
    DepthStencilDescription CreateDefaultDepthStencilDescription()
    {
        return ogle_create_def_depth_stencil_desc();
    }
    
    DepthStencilState CreateDefaultDepthState()
    {
        return ogle_create_def_depth_state(&ogs);
    }
    
    void APInit()
    {
        ogle_api_init(&ogs);
    }
    
    void Init()
    {
        ogle_init(&ogs);
    }

    void GLDeleteTexture(GLTexture* texture)
    {
        ogle_delete_texture(&ogs,texture);
    }
    
    //NOTE(Ray)IMPORTANT:This must be used in a thread safe only section of code
    //Actually this should only be used in one place that I can think of.  Kind of a dumb
    //function tbh
    static inline uint64_t GLEMuGetNextTextureID()
    {
        return ogle_next_tex_id(&ogs);
    }
    
    bool GLIsValidTexture(GLTexture texture)
    {
        return ogle_is_valid_texture(&ogs,texture);
    }
    
    void CheckPurgeTextures()
    {
        ogle_purge_textures(&ogs);
    }
    
    SamplerState GetSamplerStateWithDescriptor(SamplerDescriptor desc)
    {
        return ogle_get_sample_state(desc);
    }
    
    //Draw call and bindings
    UniformBindingTableEntry GetUniEntryForDrawCall(uint32_t index)
    {
        return ogle_get_uniform_entry_for_draw(&ogs,index);
    }
    //end Draw
    
    SamplerDescriptor GetSamplerDescriptor()
    {
        return ogle_get_sampler_desc(&ogs);
    }
    
    SamplerDescriptor GetDefaultDescriptor()
    {
        return ogle_get_def_sampler_desc(&ogs);
    }
    
    SamplerState GetDefaultSampler()
    {
        return ogle_get_def_sampler(&ogs);
    }
    
    //Depth and Stencil
    DepthStencilDescription GetDefaultDepthStencilDescriptor()
    {
        return ogle_get_def_depth_Sten_desc(&ogs);
    }
    
    DepthStencilState GetOrCreateDepthStencilState(DepthStencilDescription desc)
    {
        return ogle_get_depth_sten_state(&ogs,desc);
    }
    
    //Buffers
    void CreateBufferAtBinding(uint64_t bindkey)
    {
        ogle_create_buffer(&ogs,bindkey);
    }
    
    void AddBindingToBuffer(uint64_t buffer_key,uint64_t key)
    {
        ogle_add_buffer_binding(&ogs,buffer_key,key);
    }
    
    TripleGPUBuffer* GetBufferAtBinding(uint64_t bindkey)
    {
        return ogle_get_buffer_binding(&ogs,bindkey);
    }
    
    YoyoVector GetBufferList()
    {
        return ogle_get_buf_list(&ogs);
    }
    
    YoyoVector GetProgramList()
    {
        return ogle_get_program_list(&ogs);
    }
    
    YoyoVector GetTextureList()
    {
        return ogle_get_texture_list(&ogs);
    }
    
    void AddFragTextureBinding(GLTexture texture,uint32_t tex_index,uint32_t sampler_index)
    {
        ogle_bind_texture_frag_sampler(&ogs,texture,tex_index,sampler_index);
    }
    
    void AddFragTextureBinding(GLTexture texture,uint32_t index)
    {
        ogle_bind_texture_frag(&ogs,texture,index);
    }
    
    //TODO(Ray):Add and test this someday
    /*
        void AddVertTextureBinding(Texture texture,uint32_t index)
        {
            FragmentShaderTextureBindingTableEntry entry;
            entry.index = index;
            entry.texture = texture;
            YoyoStretchPushBack(&currently_bound_frag_textures,entry);
            float2 start_count = range_of_current_bound_frag_textures;
            start_count += float2(0,1); 
            range_of_current_bound_frag_textures = start_count;
        }
    */
    
    void AddBufferBinding(uint64_t bind_key,uint64_t index,uint64_t offset)
    {
        ogle_bind_buffer(&ogs,bind_key,index,offset);
    }
    
    //CPU Only buffers mainly for uniforms
    void CreateCPUBufferAtBinding(uint64_t bindkey,memory_index size)
    {
        ogle_create_cpu_buf_bind(&ogs,bindkey,size);
    }
    
    //Add a key to the list pointing to the buffer that is previously allocated.
    //if the key is a duplicate no need to add it.
    void AddCPUBindingToBuffer(uint64_t buffer_key,uint64_t key)
    {
        ogle_bind_cpubuffer(&ogs,buffer_key,key);
    }
    
    CPUBuffer* GetCPUBufferAtBinding(uint64_t bindkey)
    {
        return ogle_cpubuffer_at_binding(&ogs,bindkey);
    }
    
    YoyoVector GetCPUBufferList()
    {
        return ogle_get_buf_list(&ogs);
    }

    UniformBindResult AddUniformDataAtBinding(uint64_t bindkey,void* uniform_data,memory_index size)
    {
        return ogle_add_uniform_data_at_binding(&ogs,bindkey,uniform_data,size);
    }
    
    BufferOffsetResult GetUniformAtBinding(uint64_t bindkey,uint32_t index)
    {
        return ogle_get_uniform_at_binding(&ogs,bindkey,index);
    }
   
    //Shaders
    GLProgram GetDefaultProgram()
    {
        return ogle_get_def_program(&ogs);
    }

    GLProgram AddProgramFromSource(const char* v_s,const char* vs_name,const char* f_s,const char* fs_name,VertexDescriptor vd)
    {
        return ogle_add_prog_source(&ogs,v_s,vs_name,f_s,fs_name,vd);
    }

    GLProgram AddProgramFromMainLibrary(const char* vs_name,const char* fs_name,VertexDescriptor vd)
    {
        return ogle_add_prog_lib(&ogs,vs_name,fs_name,vd);
    }
    
    GLProgram GetProgram(GLProgramKey key)
    {
        return ogle_get_program(&ogs,key);
    }
    
    GLProgram* GetProgramPtr(GLProgramKey key)
    {
        return ogle_get_prog_ptr(&ogs,key);
    }
    
    uint32_t GetDepthStencilStateCount()
    {
        return ogle_get_depth_sten_state_count(&ogs);
    }
    
    DepthStencilDescription GetCurrentDepthStencilState()
    {
        return ogle_get_current_depth_sten_state(&ogs);
    }
    
    uint32_t GetCurrentStencilReferenceValue()
    {
        return ogle_get_current_stencil_ref_value(&ogs);
    }
    
    UniformBindResult AddUniData(uint32_t buffer_binding,uint32_t data_index,GLProgram* p,uint32_t size)
    {
        return ogle_add_uniform_data(&ogs,buffer_binding,data_index,p,size);
    }
    
    GLTexture TexImage2D(void* texels,float2 dim,PixelFormat format,SamplerDescriptor sd,TextureUsage usage)
    {
        return ogle_tex_image_2d_with_sampler(&ogs,texels,dim,format,sd,usage);
    }
    
    GLTexture TexImage2D(void* texels,float2 dim,PixelFormat format,TextureUsage usage)
    {
        return ogle_tex_image_2d(&ogs,texels,dim,format,usage);
    }
    
    //NOTE(Ray):No uniform state can be larger than 2kb limitation of the SetVertexBytes API in METAL
    //NOTE(Ray):Unlike gl We provide the last known state of the uniform data to the user so they can manipulate
    //it how they see fit at each state more explicitely.
    void* SetUniformsFragment_(memory_index size)
    {
        return ogle_frag_set_uniform_(&ogs,size);
    }
    
    void* SetUniformsVertex_(memory_index size)
    {

        return ogle_vert_set_uniform_(&ogs, size);
    }
    
    //NOTE(Ray):No sparse entries in tables.
    uint32_t AddDrawCallEntry(BufferOffsetResult v_uni_bind,BufferOffsetResult f_uni_bind,BufferOffsetResult tex_binds)
    {
        return ogle_add_draw_call_entry(&ogs,v_uni_bind,f_uni_bind,tex_binds);
    }
    
    void EndDraw(uint32_t unit_size)
    {
        ogle_end_draw(&ogs,unit_size);
    }
    
    void PreFrameSetup()
    {
        ogle_pre_frame_setup(&ogs);        
    }
    
    //Commands    
    static inline void AddHeader(GLEMUBufferState type)
    {
        ogle_add_header(&ogs,type);
    }
    
#define AddCommand(type) (type*)AddCommand_(sizeof(type));
    static inline void* AddCommand_(uint32_t size)
    {
        return ogle_add_command_(&ogs, size);
    }
    
#define Pop(ptr,type) (type*)Pop_(ptr,sizeof(type));ptr = (uint8_t*)ptr + (sizeof(type));
    static inline void*  Pop_(void* ptr,uint32_t size)
    {
        return ptr;
    }
    
    void GLBlending(uint64_t gl_src,uint64_t gl_dst)
    {
        ogle_blend(&ogs,gl_src,gl_dst);
    }
    
    void UseProgram(GLProgram gl_program)
    {
        ogle_use_program(&ogs,gl_program);
    }
    
    void EnableScissorTest()
    {
        ogle_enable_scissor_test(&ogs);        
    }
    
    void DisableScissorTest()
    {
        ogle_disable_scissor_test(&ogs);
    }
    
    void ScissorTest(int x,int y,int width,int height)
    {
        ogle_scissor_test(&ogs,x,y,width,height);
    }
    
    void ScissorTest(float4 rect)
    {
        ogle_scissor_test_f4(&ogs,rect);
    }
    
    void Viewport(int x, int y, int width, int height)
    {
        ogle_viewport(&ogs,x,y,width,height);
    }
    void Viewport(float4 vp)
    {
        ogle_viewport_f4(&ogs,vp);
    }
    
    void BindFrameBufferStart(GLTexture texture)
    {
        ogl_bind_framebuffer_start(&ogs,texture);        
    }
    
    void BindFrameBufferEnd()
    { 
        ogle_bind_framebuffer_end(&ogs);
    }
    
    void EnableStencilTest()
    {
        ogle_enable_stencil_test(&ogs);
    }
    
    void DisableStencilTest()
    {
        ogle_disable_stencil_test(&ogs);
    }
    
    void StencilMask(uint32_t mask)
    {
        ogle_stencil_mask(&ogs,mask);
    }
    
    void StencilMaskSep(uint32_t front_or_back,uint32_t mask)
    {
        ogle_stencil_mask_sep(&ogs,front_or_back,mask);
    }
    
    void StencilFunc(CompareFunc func,uint32_t ref,uint32_t mask)
    {
        ogle_stencil_func(&ogs,func,ref,mask);
    }
    
    void StencilFuncSep(uint32_t front_or_back,CompareFunc func,uint32_t ref,uint32_t mask)
    {
        ogle_stencil_func_sep(&ogs, front_or_back,func,ref, mask);
    }
    
    void StencilOperation(StencilOp sten_fail,StencilOp dpfail,StencilOp dppass)
    {
        ogle_stencil_op(&ogs,sten_fail,dpfail,dppass);
    }
    
    void StencilOperationSep(uint32_t front_or_back,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass)
    {
        ogle_stencil_op_sep(&ogs,front_or_back,sten_fail,dpfail,dppass);
    }
    
    void StencilFuncAndOp(CompareFunc func,uint32_t ref,uint32_t mask,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass)
    {
        ogle_stencil_func_and_op(&ogs,func, ref, mask,sten_fail, dpfail, dppass);
    }
    
    void StencilFuncAndOpSep(uint32_t front_or_back,CompareFunc func,uint32_t ref,uint32_t mask,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass)
    {
        ogle_stencil_Func_and_op_sep(&ogs,front_or_back,func,ref, mask,sten_fail, dpfail, dppass);
    }
    
    void ClearBuffer(uint32_t buffer_bits)
    {
        ogle_clear_buffer(&ogs,buffer_bits);
    }

    void ClearStencil(uint32_t value)
    {
        ogle_clear_stencil(&ogs, value);
    }
    
    void ClearColor(float4 value)
    {
        ogle_clear_color(&ogs,value);
    }
    
    void ClearColorAndStencil(float4 color,uint32_t stencil)
    {
        ogle_clear_color_and_stencil(&ogs,color, stencil);
    }
    
    //Debug calls
    void AddDebugSignPost(char* str)
    {
        ogle_add_debug_signpost(&ogs,str);
    }
    
    //NOTE(Ray):We now will reference the draw table when we add a draw and when we dispatch one
    //We grab an table entry to get all the relevant uniform data for draw call and all the
    //other info like texture bindings etc... for now at the time of writing this comment we only need to
    //keep uniforms and maybe texture bindings to complete the current project.
    //TODO(Ray):These will clean up nice once we get uniform variable support
    void DrawArrays(uint32_t current_count,uint32_t unit_size)
    {
        ogle_draw_arrays(&ogs,current_count, unit_size);        
    }
    
    void DrawArrayPrimitives(uint32_t current_count,uint32_t unit_size)
    {
        ogle_draw_array_primitives(&ogs,current_count,unit_size);
    }
    
    void Execute(void* pass_in_c_buffer)
    {
        ogle_execute(&ogs,pass_in_c_buffer);
    }
};

#endif

