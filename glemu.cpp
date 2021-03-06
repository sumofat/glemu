//NOTE(Ray):GLEMU implementation file is seperated for reading convience only
#ifdef YOYOIMPL

namespace OpenGLEmu
{
    GLEMURenderCommandList command_list;
    //Samplers
    SamplerDescriptor defaults;
    SamplerDescriptor samplerdescriptor;
    SamplerState default_sampler_state;
    
    DepthStencilDescription default_depth_stencil_description;
    DepthStencilDescription current_depth_stencil_description;
    
    DepthStencilDescription ds;
    uint32_t current_reference_value;

    bool is_stencil_enabled;
    DepthStencilState default_depth_stencil_state;
    DepthStencilState curent_depth_stencil_state;
    
    RenderShader default_shader;
    GLProgram default_program;
    GLProgram current_program;
    
    AnythingCache buffercache;
    //NOTE(Ray):intention was variable sized buffers for uniforms but decided on not using
    //Moving to fixed size vectors for each shader set since there should be only one set of uniforms
    //per shader. simpler easier but will keep cpubuffers around for variable size implementation
    //later which we will sure use later for something else.
    AnythingCache cpubuffercache;
    AnythingCache programcache;
    
    AnythingCache depth_stencil_state_cache;//We cache the depthstates here so we dont create any more than neccessarry.
    //the amount of these will probably be limited so once created we will probably wont need to free as far as I can tell.
    AnythingCache gl_texturecache;//In order to properly handle deleted textures across multiple objects we offer a way to query for and tracking of ..
    //delted adn allocated textures. 
    
    memory_index default_buffer_size;
    
    DrawCallTables draw_tables;
    ResourceManagementTables resource_managment_tables;
    
    uint32_t uniform_buffer_bindkey = 0;//OPENGLEMU reserves the zero bind key for cpu bindings no one else can use this key for cpu bindings.
    
    YoyoVector currently_bound_buffers;
    YoyoVector currently_bound_frag_textures;
    
    float2 range_of_current_bound_buffers;//x start index into currentbound buffers and y is count to iterate over.
    float2 range_of_current_bound_frag_textures;//x start index into currentbound buffers and y is count to iterate over.
    
    GLTexture default_texture;
    float4 default_tex_data[4];
    
    uint32_t draw_index;
    //Mutexes
    TicketMutex texture_mutex;
    TicketMutex buffer_mutex;////NOTE(Ray):current not used    
    
    //Used to be spritebatch needs a new home
    uint32_t current_buffer_index;
	uint32_t buffer_count;
    uint32_t buffer_size;
    
    DispatchSemaphoreT semaphore;
    
    GPUBuffer buffer[3];
    MemoryArena arena[3];
    
    GPUBuffer atlas_index_buffer[3];
    MemoryArena atlas_index_arena[3];
    
    //??
    //    GPUBuffer matrix_variable_size_buffer[3];
    //    MemoryArena matrix_variable_size_arena[3];
    
    //matrix buffer add on for those who might need it.
    GPUBuffer matrix_buffer;//uniform
    MemoryArena matrix_buffer_arena;
    bool debug_out_program_change = false;
    bool debug_out_high = false;
    bool debug_out_general = true;
    bool debug_out_uniforms = false;
    bool debug_out_signpost = true;
    RenderPipelineState prev_frame_pipeline_state;
    RenderPipelineState default_pipeline_state;
    Texture default_depth_stencil_texture;
    
    void VerifyCommandBuffer(GLEMUBufferState state)
    {
        u32 current_command_index = 0;
        uint32_t index = 0;
        while (index < command_list.count)
        {
            GLEMUCommandHeader* header = (GLEMUCommandHeader*)command_list.buffer.base;
            GLEMUBufferState command_type = header->type;
            if(index == command_list.count)
            {
                Assert(state == header->type);                
            }
            ++index;
        }
    }
    
    VertexDescriptor CreateDefaultVertexDescriptor()
    {
        VertexDescriptor vertex_descriptor = RenderEncoderCode::NewVertexDescriptor();
        //vertesx descriptors
        VertexAttributeDescriptor vad;
        vad.format = VertexFormatFloat3;
        vad.offset = 0;
        vad.buffer_index = 0;
        RenderEncoderCode::AddVertexAttribute(&vertex_descriptor,vad);
        VertexAttributeDescriptor vcad;
        vcad.format = VertexFormatFloat4;
        vcad.offset = float3::size();
        vcad.buffer_index = 0;
        RenderEncoderCode::AddVertexAttribute(&vertex_descriptor,vcad);
        VertexAttributeDescriptor uv_ad;
        uv_ad.format = VertexFormatFloat2;
        uv_ad.offset = float3::size() + float4::size();
        uv_ad.buffer_index = 0;
        RenderEncoderCode::AddVertexAttribute(&vertex_descriptor,uv_ad);
        //vertex layouts
        VertexBufferLayoutDescriptor vbld;
        vbld.step_function = step_function_per_vertex;
        vbld.step_rate = 1;
        vbld.stride = float3::size() + float4::size() + float2::size();
        RenderEncoderCode::AddVertexLayout(&vertex_descriptor,vbld);
        return vertex_descriptor;
    }
    
    DepthStencilDescription CreateDefaultDepthStencilDescription()
    {
        DepthStencilDescription depth_desc = RendererCode::CreateDepthStencilDescriptor();
        depth_desc.depthWriteEnabled = false;
        depth_desc.depthCompareFunction = compare_func_always;
        return depth_desc;
    }
    
    DepthStencilState CreateDefaultDepthState()
    {
        DepthStencilState depth_state = RendererCode::NewDepthStencilStateWithDescriptor(&default_depth_stencil_description);
        AnythingCacheCode::AddThing(&depth_stencil_state_cache,&default_depth_stencil_description,&depth_state);
        return depth_state;    
    }
    
    YoyoVector temp_deleted_tex_entries;
    uint64_t glemu_tex_id;
    
    //Need to initialize these.
    RenderPassDescriptor default_render_pass_descriptor;
    RenderPassBuffer pass_buffer;
    
    void APInit()
    {
        command_list.buffer = PlatformAllocatePartition(MegaBytes(20));
        is_stencil_enabled = false;
        float2 dim = RendererCode::dim;
        
        TextureDescriptor depth_texture_desc = RendererCode::Texture2DDescriptorWithPixelFormat(PixelFormatDepth32Float_Stencil8,dim.x(),dim.y(),false);
        depth_texture_desc.usage       = TextureUsageRenderTarget;
        depth_texture_desc.storageMode = StorageModePrivate;
        depth_texture_desc.pixelFormat = PixelFormatDepth32Float_Stencil8;
        
        Texture depth_texture = RendererCode::NewTextureWithDescriptor(depth_texture_desc);
        RenderPassDescriptor sp_rp_desc = RenderEncoderCode::NewRenderPassDescriptor();
        //depth attachment description
        sp_rp_desc.depth_attachment.description.texture       = depth_texture;
        sp_rp_desc.depth_attachment.description.loadAction    = LoadActionClear;
        sp_rp_desc.depth_attachment.description.storeAction   = StoreActionStore;
        sp_rp_desc.depth_attachment.clear_depth               = 1.0f;
        
        //stencil attachment description
        sp_rp_desc.stencil_attachment.description.texture     = depth_texture;
        sp_rp_desc.stencil_attachment.description.loadAction  = LoadActionClear;
        sp_rp_desc.stencil_attachment.description.storeAction = StoreActionStore;
        sp_rp_desc.stencil_attachment.clearStencil = 0.0f;
        
        sp_rp_desc.depth_attachment = sp_rp_desc.depth_attachment;
        sp_rp_desc.stencil_attachment = sp_rp_desc.stencil_attachment;
        RendererCode::SetRenderPassDescriptor(&sp_rp_desc);
        
        RenderPassColorAttachmentDescriptor sprite_rp_ca_desc = {};
        sprite_rp_ca_desc.clear_color = float4(0.392f,0.584f,0.929f,1);//corflower blue of course
        sprite_rp_ca_desc.description.loadAction = LoadActionLoad;
        sprite_rp_ca_desc.description.storeAction = StoreActionStore;
        RenderEncoderCode::AddRenderPassColorAttachment(&sp_rp_desc,&sprite_rp_ca_desc);
        RenderEncoderCode::SetRenderPassColorAttachmentDescriptor(&sp_rp_desc,0);
        
        default_render_pass_descriptor = sp_rp_desc;
        
        RenderMaterial material = SpriteBatchCode::CreateSpriteBatchMaterials("spritebatch_vs_matrix_indexed","spritebatch_fs_single_sampler","Test matrix buffer");
        
        default_pipeline_state = material.pipeline_state;
        for(int i = 0;i < buffer_count;++i)
        {
            uint32_t size = (uint32_t)buffer_size;
            buffer[i] = RenderGPUMemory::NewBufferWithLength(size,ResourceStorageModeShared);
            //Assert(buffer[i].buffer);
            arena[i] = AllocatePartition(size,buffer[i].data);
            
            //NOTE(RAY):Same of max sprites for this sprite batch perhaps we should just take and set the max number of sprites
            //to be used per batch or capacity as another way to put it.
            uint32_t atlas_index_buffer_size = (size / SIZE_OF_SPRITE_IN_BYTES) * sizeof(uint32_t);
            atlas_index_buffer[i] = RenderGPUMemory::NewBufferWithLength(atlas_index_buffer_size,ResourceStorageModeShared);
            //Assert(atlas_index_buffer[i].buffer);
            atlas_index_arena[i] = AllocatePartition(atlas_index_buffer_size,atlas_index_buffer[i].data);
            //            matrix_variable_size_arena[i] = AllocatePartition(atlas_index_buffer_size,matrix_variable_size_buffer[i].data);
        }
        
        uint32_t size = (uint32_t)buffer_size;
        matrix_buffer = RenderGPUMemory::NewBufferWithLength(size,ResourceStorageModeShared);
        Assert(matrix_buffer.buffer);
        matrix_buffer_arena = AllocatePartition(size,matrix_buffer.data);
    }
    
    void Init()
    {
        RenderCache::Init(MAX_PSO_STATES);
        
        glemu_tex_id = 0;

        //TODO(Ray):Make this resizable.  GIve more control to the user over the allocation space usage
        //lifetimes without too much hassle.
//        default_buffer_size = MegaBytes(1);
        default_buffer_size = MegaBytes(2);
        
        buffer_count = 3;
        buffer_size = 1024 * SIZE_OF_SPRITE_IN_BYTES;
        temp_deleted_tex_entries = YoyoInitVector(1,GLTextureKey,false);
        
        semaphore = RenderSynchronization::DispatchSemaphoreCreate(buffer_count);

        //NOTE(RAY):We are intentionally using a lower number here to create collisions while testing and ensuring hashtable
        //is robust.  Increase this number to pretty much make sure collisions will be fewer if at all.
        // The typical storage requirement is 4 bytes (64bits) per as the
        //storage is of uint64.
#define SIZE_OF_CACHE_TABLES 9973
#define SIZE_OF_CACHE_TABLES_SMALL 9973 /// probably large enough for most small use cache to avoid collisions
        AnythingRenderSamplerStateCache::Init(SIZE_OF_CACHE_TABLES_SMALL);
        AnythingCacheCode::Init(&buffercache,SIZE_OF_CACHE_TABLES_SMALL,sizeof(TripleGPUBuffer),sizeof(uint64_t));
        AnythingCacheCode::Init(&programcache,SIZE_OF_CACHE_TABLES,sizeof(GLProgram),sizeof(GLProgramKey));
        AnythingCacheCode::Init(&cpubuffercache,SIZE_OF_CACHE_TABLES,sizeof(CPUBuffer),sizeof(uint64_t));
        AnythingCacheCode::Init(&depth_stencil_state_cache,SIZE_OF_CACHE_TABLES,sizeof(DepthStencilState),sizeof(DepthStencilDescription));
        AnythingCacheCode::Init(&gl_texturecache,SIZE_OF_CACHE_TABLES,sizeof(GLTexture),sizeof(GLTextureKey),true);
        
        samplerdescriptor = RendererCode::CreateSamplerDescriptor();
        defaults = samplerdescriptor;
        
        VertexDescriptor vd = CreateDefaultVertexDescriptor();
        default_program = AddProgramFromMainLibrary("spritebatch_vs_matrix_indexed","spritebatch_fs_single_sampler",vd);
        default_shader = default_program.shader;
        default_depth_stencil_description = CreateDefaultDepthStencilDescription();
        default_depth_stencil_state = CreateDefaultDepthState();
        
        //create default uniform cpu buffer
        CreateCPUBufferAtBinding(uniform_buffer_bindkey,MegaBytes(10));        
        
        draw_tables.uniform_binding_table = YoyoInitVector(1,UniformBindingTableEntry,false);
        draw_tables.texture_binding_table = YoyoInitVector(1,TextureBindingTableEntry,false);
        draw_tables.buffer_binding_table = YoyoInitVector(1,BufferBindingTableEntry,false);
        
        resource_managment_tables = {};
        AnythingCacheCode::Init(&resource_managment_tables.released_textures_table,SIZE_OF_CACHE_TABLES,sizeof(ReleasedTextureEntry),sizeof(GLTextureKey),true);
        
        currently_bound_buffers = YoyoInitVector(1,BufferBindingTableEntry,false);
        currently_bound_frag_textures = YoyoInitVector(1,FragmentShaderTextureBindingTableEntry,false);
        
        range_of_current_bound_buffers = float2(0.0f);
        range_of_current_bound_frag_textures = float2(0.0f);
        
        float2 dim = float2(2,2);
        float4 b = float4(0.0f);
        for(int i = 0;i < 4;++i)
        {
            default_tex_data[i] = b;                    
        }
        default_texture = TexImage2D(&default_tex_data,dim,PixelFormatRGBA8Unorm,TextureUsageShaderRead);
        default_texture.sampler = GetDefaultSampler();
        OpenGLEmu::APInit();
    }
    
    //NOTE(Ray):So when you delete a texture we mark it deleted and add to a deleted table...
    //Everytime a texture is used for a frame we keep it around for n frames... if it gets used the counter resets...
    //If it is never used after N frames limit we actually release it.
    //TODO(Ray):Later what we could do here is result a texture if it matches the dimensions of another texture.
    //than if the amount of unused textures grows beyone a certain limit release the resources back to the GPU.
    //Could use resource heaps if we really want to ball before now... just doing the easiest thing.
    //For now we just do the N frame counting scheme and revisit this later.
    void GLDeleteTexture(GLTexture* texture)
    {
        BeginTicketMutex(&texture_mutex);
        GLTextureKey ttk = {};

        ttk.format = texture->texture.descriptor.pixelFormat;
        ttk.width = texture->texture.descriptor.width;
        ttk.height = texture->texture.descriptor.height;
        ttk.sample_count = texture->texture.descriptor.sampleCount;
        ttk.storage_mode = texture->texture.descriptor.storageMode;
        //ttk.allowGPUOptimizedContents = texture->texture.descriptor.allowGPUOptimizedContents;
        ttk.gl_tex_id = texture->id;
        
        //If we are not in the texturecache cant delete it since its not a texture we know about.
        //And is not already been added to the released textures table.
        if(AnythingCacheCode::DoesThingExist(&gl_texturecache,&ttk))
        {
            if(!AnythingCacheCode::DoesThingExist(&resource_managment_tables.released_textures_table,&ttk))
            {
                GLTexture* tex = GetThingPtr(&gl_texturecache,&ttk,GLTexture);
                Assert(!texture->texture.is_released);
                if(!tex->texture.is_released)
                {
                    Assert(!tex->texture.is_released);
                    ReleasedTextureEntry rte = {};
                    rte.tex_key = ttk;
                    rte.delete_count = GLEMU_DEFAULT_TEXTURE_DELETE_COUNT;
                    rte.current_count = 0;
                    rte.thread_id = YoyoGetThreadID();
                    PlatformOutput(true,"BeginDeleteTexture id %d generated on thread %d on thread %d ??n",texture->id,texture->gen_thread,rte.thread_id);
                    
                    rte.is_free = false;
//                    tex->texture.is_released = true;
                    AnythingCacheCode::AddThingFL(&resource_managment_tables.released_textures_table,&ttk,&rte);
                }
            }
        }
        EndTicketMutex(&texture_mutex);
    }
    
    //NOTE(Ray)IMPORTANT:This must be used in a thread safe only section of code
    //Actually this should only be used in one place that I can think of.  Kind of a dumb
    //function tbh
    static inline uint64_t GLEMuGetNextTextureID()
    {
        return ++glemu_tex_id;
    }
    
    bool GLIsValidTexture(GLTexture texture)
    {
        
        bool result = false;
        BeginTicketMutex(&texture_mutex);                                
        GLTextureKey ttk = {};
        ttk.format = texture.texture.descriptor.pixelFormat;
        ttk.width = texture.texture.descriptor.width;
        ttk.height = texture.texture.descriptor.height;
        ttk.sample_count = texture.texture.descriptor.sampleCount;
        ttk.storage_mode = texture.texture.descriptor.storageMode;
        //ttk.allowGPUOptimizedContents = texture.texture.descriptor.allowGPUOptimizedContents;
        ttk.gl_tex_id = texture.id;
        
        if(!AnythingCacheCode::DoesThingExist(&gl_texturecache,&ttk))
        {
            result = false;
        }
        else if(!AnythingCacheCode::DoesThingExist(&resource_managment_tables.released_textures_table,&ttk))
        {
            result = true;
        }
        else
        {
            result = false;
        }
        EndTicketMutex(&texture_mutex);        
        return result;        
    }
    
    void CheckPurgeTextures()
    {
        //NOTE(RAY):Making sure that we do not add or remove textures at this point or beyond.
        BeginTicketMutex(&texture_mutex);
        int pop_count = 0;
        for(int i = 0;i < resource_managment_tables.released_textures_table.anythings.count;++i)
        {
            ReleasedTextureEntry* rte = YoyoGetVectorElement(ReleasedTextureEntry,&resource_managment_tables.released_textures_table.anythings,i);
            if(rte)
            {
                ++rte->current_count;
                if(rte->current_count >= rte->delete_count  && rte->is_free == false)
                {
                    if(AnythingCacheCode::DoesThingExist(&gl_texturecache,&rte->tex_key))
                    {
                        GLTexture* tex = GetThingPtr(&gl_texturecache,&rte->tex_key,GLTexture);
                        if(tex->texture.descriptor.usage == 5)
                        {
                            continue;
                        }
                        Assert(tex->id == rte->tex_key.gl_tex_id);
                        Assert(tex->texture.state);
                        Assert(!tex->texture.is_released);
                        Assert(!tex->is_released);
//NOTE(RAY):There is some unknown crash inside set purgable state //TODO(Ray):Need to figure out what is causing it.
                        PlatformOutput(true,"Finalalize delete texture id %d generated on thread %d on thread %d \n",tex->id,tex->gen_thread,rte->thread_id);
                        RendererCode::ReleaseTexture(&tex->texture);
                        tex->is_released = true;
                        tex->texture.is_released = true;
                    }
#if GLEMU_DEBUG
                    else
                    {
                        Assert(false);
                    }
#endif
                    YoyoStretchPushBack(&temp_deleted_tex_entries,rte->tex_key);
                    rte->tex_key = {};
                    rte->thread_id = 0;
                    rte->current_count = 0;
                    rte->is_free = true;
                }                
            }
        }
        
        for(int i = 0;i < temp_deleted_tex_entries.count;++i)
        {
            GLTextureKey* tkey = (GLTextureKey*)temp_deleted_tex_entries.base + i;
            AnythingCacheCode::RemoveThingFL(&resource_managment_tables.released_textures_table,tkey);
            AnythingCacheCode::RemoveThingFL(&gl_texturecache,tkey);
        }
        
        YoyoClearVector(&temp_deleted_tex_entries);
        EndTicketMutex(&texture_mutex);
    }
    
    SamplerState GetSamplerStateWithDescriptor(SamplerDescriptor desc)
    {
        SamplerState* result;
        if(AnythingRenderSamplerStateCache::DoesSamplerStateExists(&desc))
        {
            result = AnythingRenderSamplerStateCache::GetSamplerState(&desc);
        }
        else
        {
            SamplerState new_sampler_state = RenderGPUMemory::NewSamplerStateWithDescriptor(&desc);
            AnythingRenderSamplerStateCache::AddSamplerState(&desc,&new_sampler_state);
            result = AnythingRenderSamplerStateCache::GetSamplerState(&desc);
        }
        //Assert(result);
        return (*result);                
    }
    
    //Draw call and bindings
    UniformBindingTableEntry GetUniEntryForDrawCall(uint32_t index)
    {
        UniformBindingTableEntry result = {};
        UniformBindingTableEntry* entry = YoyoGetVectorElement(UniformBindingTableEntry,&draw_tables.uniform_binding_table,index);
        if(entry)
        {
            result = *entry;
        }
        return result;
    }
    //end Draw
    
    SamplerDescriptor GetSamplerDescriptor()
    {
        return samplerdescriptor;        
    }
    
    SamplerDescriptor GetDefaultDescriptor()
    {
        return defaults;
    }
    
    SamplerState GetDefaultSampler()
    {
        default_sampler_state = GetSamplerStateWithDescriptor(defaults);
        return default_sampler_state;        
    }
    
    //Depth and Stencil
    DepthStencilDescription GetDefaultDepthStencilDescriptor()
    {
        return default_depth_stencil_description;
    }
    
    DepthStencilState GetOrCreateDepthStencilState(DepthStencilDescription desc)
    {
        DepthStencilState result = {};
        if(AnythingCacheCode::DoesThingExist(&depth_stencil_state_cache,&desc))
        {
            result = GetThingCopy(&depth_stencil_state_cache,(void*)&desc,DepthStencilState);
        }
        else
        {
            result = RendererCode::NewDepthStencilStateWithDescriptor(&desc);
            result.desc = desc;
            AnythingCacheCode::AddThing(&depth_stencil_state_cache,(void*)&desc,&result);            
        }
        
        Assert(desc.depthCompareFunction == result.desc.depthCompareFunction);
        Assert(desc.depthWriteEnabled == result.desc.depthWriteEnabled);
        Assert(desc.backFaceStencil.stencilFailureOperation == result.desc.backFaceStencil.stencilFailureOperation);
        Assert(desc.backFaceStencil.depthFailureOperation == result.desc.backFaceStencil.depthFailureOperation);
        Assert(desc.backFaceStencil.stencilCompareFunction == result.desc.backFaceStencil.stencilCompareFunction);
        Assert(desc.backFaceStencil.write_mask == result.desc.backFaceStencil.write_mask);
        Assert(desc.backFaceStencil.read_mask == result.desc.backFaceStencil.read_mask);
        Assert(desc.backFaceStencil.enabled == result.desc.backFaceStencil.enabled);
        
        Assert(desc.frontFaceStencil.stencilFailureOperation == result.desc.frontFaceStencil.stencilFailureOperation);
        Assert(desc.frontFaceStencil.depthFailureOperation == result.desc.frontFaceStencil.depthFailureOperation);
        Assert(desc.frontFaceStencil.stencilCompareFunction == result.desc.frontFaceStencil.stencilCompareFunction);
        Assert(desc.frontFaceStencil.write_mask == result.desc.frontFaceStencil.write_mask);
        Assert(desc.frontFaceStencil.read_mask == result.desc.frontFaceStencil.read_mask);
        Assert(desc.frontFaceStencil.enabled == result.desc.frontFaceStencil.enabled);
        return result;
    }
    
    //Buffers
    void CreateBufferAtBinding(uint64_t bindkey)
    {
        TripleGPUBuffer buffer = {};
        for(int i = 0;i < 3;++i)
        {
            uint32_t size = (uint32_t)default_buffer_size;
            buffer.buffer[i] = RenderGPUMemory::NewBufferWithLength(size,ResourceStorageModeShared);
            Assert(buffer.buffer[i].data);
            buffer.arena[i] = AllocatePartition(size,buffer.buffer[i].data);
        }
        uint64_t tt = bindkey;
        AnythingCacheCode::AddThing(&buffercache,(void*)&tt,&buffer);
    }
    
    void AddBindingToBuffer(uint64_t buffer_key,uint64_t key)
    {
        uint64_t tt = key;
        if(!AnythingCacheCode::DoesThingExist(&buffercache,&tt))
        {
            uint64_t* ptr = YoyoGetElementByHash(uint64_t,&buffercache.hash,&buffer_key,buffercache.hash.key_size);
            YoyoAddElementToHashTable(&buffercache.hash,(void*)&tt,buffercache.key_size,ptr);
        }
    }
    
    TripleGPUBuffer* GetBufferAtBinding(uint64_t bindkey)
    {
        uint64_t tt = bindkey;
        return GetThingPtr(&buffercache,(void*)&tt,TripleGPUBuffer);        
    }
    
    YoyoVector GetBufferList()
    {
        return buffercache.anythings;
    }
    
    YoyoVector GetProgramList()
    {
        return programcache.anythings;
    }
    
    YoyoVector GetTextureList()
    {
        return gl_texturecache.anythings;
    }
    
    void AddFragTextureBinding(GLTexture texture,uint32_t tex_index,uint32_t sampler_index)
    {
        Assert(texture.texture.state);
        Assert(texture.sampler.state);
        
        if(GLIsValidTexture(texture))
        {
        }
        else
        {
            texture = default_texture;
        }
        
        FragmentShaderTextureBindingTableEntry entry = {};
        entry.tex_index = tex_index;
        entry.sampler_index = sampler_index;
        entry.texture = texture;
        
        YoyoStretchPushBack(&currently_bound_frag_textures,entry);
        float2 start_count = range_of_current_bound_frag_textures;
        start_count += float2(0,1); 
        range_of_current_bound_frag_textures = start_count;            
    }
    
    void AddFragTextureBinding(GLTexture texture,uint32_t index)
    {
        //        Assert(!texture.texture.is_released);
        Assert(texture.texture.state);
        Assert(texture.sampler.state);
        if(GLIsValidTexture(texture))        
        {
        }
        else
        {
            texture = default_texture;    
        }
        
        FragmentShaderTextureBindingTableEntry entry = {};
        entry.tex_index = index;
        entry.sampler_index = index;
        entry.texture = texture;
        
        YoyoStretchPushBack(&currently_bound_frag_textures,entry);
        float2 start_count = range_of_current_bound_frag_textures;
        start_count += float2(0,1); 
        range_of_current_bound_frag_textures = start_count;
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
        BufferBindingTableEntry entry = {};
        entry.key = bind_key;
        entry.offset = offset;//NOTE(Ray):For now we will just hold the hole buffer rather than a key
        entry.index = index;
        YoyoStretchPushBack(&currently_bound_buffers,entry);
        float2 start_count = range_of_current_bound_buffers;
        start_count += float2(0,1); 
        range_of_current_bound_buffers = start_count;
    }
    
    //CPU Only buffers mainly for uniforms
    void CreateCPUBufferAtBinding(uint64_t bindkey,memory_index size)
    {
        //Start size will be something basic like 10k but resize at the end a
        CPUBuffer buffer = {};
        buffer.buffer = PlatformAllocatePartition(size);
        buffer.ranges = YoyoInitVectorSize(1,float2::size(),false);
        uint64_t tt = bindkey;
        AnythingCacheCode::AddThing(&cpubuffercache,(void*)&tt,&buffer);
    }
    
    //Add a key to the list pointing to the buffer that is previously allocated.
    //if the key is a duplicate no need to add it.
    void AddCPUBindingToBuffer(uint64_t buffer_key,uint64_t key)
    {
        uint64_t tt = key;
        if(!AnythingCacheCode::DoesThingExist(&cpubuffercache,&tt))
        {
            uint64_t* ptr = YoyoGetElementByHash(uint64_t,&cpubuffercache.hash,&buffer_key,cpubuffercache.hash.key_size);
            YoyoAddElementToHashTable(&cpubuffercache.hash,(void*)&tt,cpubuffercache.key_size,ptr);
        }
    }
    
    CPUBuffer* GetCPUBufferAtBinding(uint64_t bindkey)
    {
        if(cpubuffercache.key_size > 0)
        {
            uint64_t tt = bindkey;
            return GetThingPtr(&cpubuffercache,(void*)&tt,CPUBuffer);
        }
        else
        {
            return 0;
        }
    }
    
    YoyoVector GetCPUBufferList()
    {
        return cpubuffercache.anythings;
    }

    
    UniformBindResult AddUniformDataAtBinding(uint64_t bindkey,void* uniform_data,memory_index size)
    {
        UniformBindResult result;
        
        CPUBuffer* buf = GetCPUBufferAtBinding(bindkey);
        float2 oldft = float2(0.0f);

        //This is hard to grasp make it easier to understand
        //---------
        float2* ft = YoyoPeekVectorElement(float2,&buf->ranges);
        if(ft)
        {
            oldft = *ft;
        }

        float2 newft = float2(oldft.y(),oldft.y() + size);
        YoyoStretchPushBack(&buf->ranges,newft);
        
        partition_push_params params = DefaultPartitionParams();
        params.Flags = PartitionFlag_None;
        
        //copy uni data into buffer
        //TODO(Ray):Allow for resizing
        void* dst = (void*)PushSize(&buf->buffer, size,params);
       if(uniform_data)
            memcpy(dst,uniform_data,(uint32_t)newft.y());
        
        result.ptr = dst;
        result.data_index = buf->ranges.count - 1;
        return result;
    }
    
    BufferOffsetResult GetUniformAtBinding(uint64_t bindkey,uint32_t index)
    {
        BufferOffsetResult result = {};
        CPUBuffer* buf = GetCPUBufferAtBinding(bindkey);
        float2* range_ptr = YoyoGetVectorElement(float2,&buf->ranges,index);
        if(range_ptr)
        {
            float2 range = *range_ptr;
            Assert(buf->buffer.used >= (uint32_t)range.y());
            Assert((uint32_t)range.y() <= buf->buffer.size);
            Assert(buf->buffer.used >= (uint32_t)range.x());
            Assert((uint32_t)range.x() <= buf->buffer.size);

            result.ptr = (void*)((uint8_t*)buf->buffer.base + (uint32_t)range.x());
            result.size = range.y() - range.x();
        }
        else
        {
            result.ptr = nullptr;
            result.size = 0;
        }
        return result;
    }
   
    //Shaders
    GLProgram GetDefaultProgram()
    {
        return default_program;
    }

    GLProgram AddProgramFromSource(const char* v_s,const char* vs_name,const char* f_s,const char* fs_name,VertexDescriptor vd)
    {
        RenderShader s = {};
        RenderShaderCode::InitShader(&s,(char*)v_s,(char*)vs_name,(char*)f_s,(char*)fs_name);
        GLProgram result= {};
        result.shader = s;
        result.vd = vd;
        result.last_fragment_buffer_binding = uniform_buffer_bindkey;
        result.last_fragment_data_index = 0;
        result.last_vertex_buffer_binding = uniform_buffer_bindkey;
        result.last_vertex_data_index = 0;
        GLProgramKey program_hash_key = {(uint64_t)s.vs_object,(uint64_t)s.ps_object};
        AnythingCacheCode::AddThing(&programcache,(void*)&program_hash_key,&result);
        GLProgram* p = GetProgramPtr(program_hash_key);
        return result;        
    }
    
    //NOTE(Ray):TODO(Ray):What would be nice is if we had some introspection into the shader and could get
    //out and build the vertex description from that.
    //Than use that to also drive what buffers we will need etc... for simpler and faster iteration times.
    GLProgram AddProgramFromMainLibrary(const char* vs_name,const char* fs_name,VertexDescriptor vd)
    {
        RenderShader s = {};
        RenderShaderCode::InitShaderFromDefaultLib(&s,vs_name,fs_name);
        GLProgram result = {};
        result.shader = s;
        result.vd = vd;
        result.last_fragment_buffer_binding = uniform_buffer_bindkey;
        result.last_fragment_data_index = 0;
        result.last_vertex_buffer_binding = uniform_buffer_bindkey;
        result.last_vertex_data_index = 0;

        GLProgram* p;
        GLProgramKey program_hash_key = {(uint64_t)s.vs_object,(uint64_t)s.ps_object};
        if(!AnythingCacheCode::DoesThingExist(&programcache,(void*)&program_hash_key))
        {
            AnythingCacheCode::AddThing(&programcache,(void*)&program_hash_key,&result);
        }
        else
        {
            p = (GLProgram*)AnythingCacheCode::GetThing(&programcache,(void*)&program_hash_key);
            Assert(p);
        }
        p = GetProgramPtr(program_hash_key);
        return result;
    }
    
    GLProgram GetProgram(GLProgramKey key)
    {
        return GetThingCopy(&programcache,&key,GLProgram);
    }
    
    GLProgram* GetProgramPtr(GLProgramKey key)
    {
        return GetThingPtr(&programcache,&key,GLProgram);
    }
    
    uint32_t GetDepthStencilStateCount()
    {
        return depth_stencil_state_cache.anythings.count;
    }
    
    DepthStencilDescription GetCurrentDepthStencilState()
    {
        return ds;
    }
    
    uint32_t GetCurrentStencilReferenceValue()
    {
        return current_reference_value;
    }
    
    UniformBindResult AddUniData(uint32_t buffer_binding,uint32_t data_index,GLProgram* p,uint32_t size)
    {
        BufferOffsetResult last = OpenGLEmu::GetUniformAtBinding(buffer_binding,data_index);
        return AddUniformDataAtBinding(buffer_binding,last.ptr,size);
    }
    
    GLTexture TexImage2D(void* texels,float2 dim,PixelFormat format,SamplerDescriptor sd,TextureUsage usage)
    {

        BeginTicketMutex(&texture_mutex);

        Assert(texels);
        GLTexture texture = {};
        texture.sampler = OpenGLEmu::GetDefaultSampler();
        TextureDescriptor td = {};
        td = RendererCode::Texture2DDescriptorWithPixelFormat(format,dim.x(),dim.y(),false);
#if OSX
        td.storageMode = StorageModeManaged;
#endif
        td.usage = (TextureUsage)usage;
        texture.texture = RendererCode::NewTextureWithDescriptor(td);
        //NOTE(Ray):This is ok as long as we are not releasing any descriptors but as soon as we did...
        //we have to make sure this is a valid sampler that we add here and not one scheduled for deletion.
        texture.sampler = OpenGLEmu::GetSamplerStateWithDescriptor(sd);
        if(texels)
        {
            RenderRegion region;
            region.origin = float3(0);
            region.size = dim;
            
            //TODO(Ray):We will need a more comprehensive way to check that we are passing in the proper parameters
            //to Replace REgions for packed and compressed formats and proper bytes sizes to multiply width and
            //for 1d arrays should pass in 0
            int byte_size_of_format = 4;

#if IOS && !(OSX ||  __x86_64__ || __i386__)
            if(format == PixelFormatABGR4Unorm)
                byte_size_of_format = 2;
#endif
            
            RenderGPUMemory::ReplaceRegion(texture.texture,region,0,texels,byte_size_of_format * dim.x());
        }
        
        texture.gen_thread = YoyoGetThreadID();
        texture.id = GLEMuGetNextTextureID();
        
        GLTextureKey k = {};
        k.format = texture.texture.descriptor.pixelFormat;
        k.width = texture.texture.descriptor.width;
        k.height = texture.texture.descriptor.height;
        k.sample_count = texture.texture.descriptor.sampleCount;
        k.storage_mode = texture.texture.descriptor.storageMode;
        //k.allowGPUOptimizedContents = texture.texture.descriptor.allowGPUOptimizedContents;
        k.gl_tex_id = texture.id;
        
        texture.texture.is_released = false;
        texture.is_released = false;
        
        if(!AnythingCacheCode::AddThingFL(&gl_texturecache,&k,&texture))
        {
            PlatformOutput(true,"Texture already Exist was not added to texture cache \n");
        }
        PlatformOutput(true,"TextureCreated id %d generated on thread %d on thread %d \n",texture.id,texture.gen_thread);
        
        EndTicketMutex(&texture_mutex);
        return texture;
    }
    
    GLTexture TexImage2D(void* texels,float2 dim,PixelFormat format,TextureUsage usage)
    {
        SamplerDescriptor sd = OpenGLEmu::GetDefaultDescriptor();
        sd.r_address_mode = SamplerAddressModeClampToEdge;
        sd.s_address_mode = SamplerAddressModeClampToEdge;
        sd.min_filter = SamplerMinMagFilterLinear;
        sd.mag_filter = SamplerMinMagFilterLinear;
        return TexImage2D(texels,dim,format,sd,usage);
    }
    
    //NOTE(Ray):No uniform state can be larger than 2kb limitation of the SetVertexBytes API in METAL
    //NOTE(Ray):Unlike gl We provide the last known state of the uniform data to the user so they can manipulate
    //it how they see fit at each state more explicitely.
    void* SetUniformsFragment_(memory_index size)
    {
        Assert(size <= KiloBytes(4));
        GLProgramKey key = {(uint64_t)current_program.shader.vs_object,(uint64_t)current_program.shader.ps_object};
        GLProgram* p = OpenGLEmu::GetProgramPtr(key);
        Assert(p);
        UniformBindResult result = AddUniData(p->last_fragment_buffer_binding,p->last_fragment_data_index,p,size);
        p->last_fragment_data_index = result.data_index;
        p->last_fragment_buffer_binding = p->last_fragment_buffer_binding;
        return result.ptr;
    }
    
    void* SetUniformsVertex_(memory_index size)
    {
        Assert(size <= KiloBytes(4));
        GLProgramKey key = {(uint64_t)current_program.shader.vs_object,(uint64_t)current_program.shader.ps_object};        
        GLProgram* p = OpenGLEmu::GetProgramPtr(key);
        Assert(p);
        UniformBindResult result = AddUniData(p->last_vertex_buffer_binding,p->last_vertex_data_index,p,size);
        p->last_vertex_data_index = result.data_index;
        p->last_vertex_buffer_binding = p->last_vertex_buffer_binding;
        return result.ptr;
    }
    
    //NOTE(Ray):No sparse entries in tables.
    uint32_t AddDrawCallEntry(BufferOffsetResult v_uni_bind,BufferOffsetResult f_uni_bind,BufferOffsetResult tex_binds)
    {
        UniformBindingTableEntry ue;
        ue.call_index = draw_index;
        ue.v_size = v_uni_bind.size;
        ue.v_data = v_uni_bind.ptr;
        ue.f_size = f_uni_bind.size;
        ue.f_data = f_uni_bind.ptr;
        uint32_t vf_index = YoyoStretchPushBack(&draw_tables.uniform_binding_table,ue);
        
        TextureBindingTableEntry tex_entry;
        tex_entry.call_index = draw_index;
        tex_entry.size = tex_binds.size;
        tex_entry.texture_ptr = tex_binds.ptr;
        uint32_t index = YoyoStretchPushBack(&draw_tables.texture_binding_table,tex_entry);
        
        draw_index++;
        Assert(index == vf_index);
        return index;
    }
    
    void EndDraw(uint32_t unit_size)
    {
        range_of_current_bound_buffers = float2(range_of_current_bound_buffers.y());
        range_of_current_bound_frag_textures = float2(range_of_current_bound_frag_textures.y());
        draw_index++;        
    }
    
    //Commands    
    static inline void AddHeader(GLEMUBufferState type)
    {
        GLEMUCommandHeader* header = PushStruct(&command_list.buffer,GLEMUCommandHeader);
        header->type = type;
    }
    
#define AddCommand(type) (type*)AddCommand_(sizeof(type));
    static inline void* AddCommand_(uint32_t size)
    {
        ++command_list.count;
        return PushSize(&command_list.buffer,size);
    }
    
    void PreFrameSetup()
    {
        command_list.buffer.used = 0;
        command_list.count = 0;
        
        uint32_t bi =  current_buffer_index;
        arena[bi].used = 0;
        atlas_index_arena[bi].used = 0;
        matrix_buffer_arena.used = 0;
        draw_index = 0;
        
        //reset all uniform buffers
        CPUBuffer* ub = OpenGLEmu::GetCPUBufferAtBinding(0);
        if(ub)
        {
            YoyoClearVector(&ub->ranges);
            YoyoClearVector(&ub->unit_sizes);
            ub->entry_count = 0;
            ub->buffer.used = 0;
        }
        
        //Reset shader binding back to zero
        YoyoVector pl = OpenGLEmu::GetProgramList();
        for(int i = 0;i < pl.count;++i)
        {
            GLProgram* program = (GLProgram*)pl.base + i;
            program->last_fragment_buffer_binding = 0;
            program->last_fragment_data_index = 0;
            
            program->last_vertex_buffer_binding = 0;
            program->last_vertex_data_index = 0;
        }
        
        YoyoVector bl = OpenGLEmu::GetBufferList();
        for (int i = 0; i < bl.count; ++i)
        {
            TripleGPUBuffer* buffer;
            buffer = (TripleGPUBuffer*)bl.base + i;
            buffer->from_to = float2(0.0f);
            buffer->from_to_bytes = float2(0.0f);
            buffer->arena[bi].used = 0;
            buffer->current_count = 0;
        }
        
        YoyoClearVector(&draw_tables.uniform_binding_table);
        YoyoClearVector(&draw_tables.texture_binding_table);
        YoyoClearVector(&currently_bound_buffers);
        YoyoClearVector(&currently_bound_frag_textures);
        range_of_current_bound_buffers = float2(0.0f);
        range_of_current_bound_frag_textures = float2(0.0f);
        RenderSynchronization::DispatchSemaphoreWait(&semaphore,YOYO_DISPATCH_TIME_FOREVER);
        
        //IMPORTANT(RAY):This must be done after the semaphore! Wait for the gpu to signal its done before
        //we move to the next buffer
        current_buffer_index = (current_buffer_index + 1) %  buffer_count;
        
        CheckPurgeTextures();
    }
    
    void GLBlending(uint64_t gl_src,uint64_t gl_dst)
    {
        AddHeader(glemu_bufferstate_blend_change);
        GLEMUBlendCommand* command = AddCommand(GLEMUBlendCommand);        
        uint64_t interim = gl_src;
        command->sourceRGBBlendFactor = (BlendFactor)(uint32_t)RenderGLEnum::GetMetalEnumForGLEnum(&interim);
        uint64_t interim_dest = gl_dst;
        command->destinationRGBBlendFactor = (BlendFactor)(uint32_t)RenderGLEnum::GetMetalEnumForGLEnum(&interim_dest);
    }
    
    void UseProgram(GLProgram gl_program)
    {
        AddHeader(glemu_bufferstate_shader_program_change);
        current_program = gl_program;
        Assert(gl_program.id == 0);
//        RenderPipelineStateDesc d = CreatePipelineDescriptor((void*)0,(void*)0,0);
//        RenderPipelineState rps = NewRenderPipelineStateWithDescriptor(d);
//        rps.vertexFunction
        GLEMUUseProgramCommand* command = AddCommand(GLEMUUseProgramCommand);
        command->program = gl_program;
    }
    
    void EnableScissorTest()
    {
        AddHeader(glemu_bufferstate_scissor_test_enable);
        GLEMUScissorTestCommand* command = AddCommand(GLEMUScissorTestCommand);
        command->is_enable = true;
    }
    
    void DisableScissorTest()
    {
        AddHeader(glemu_bufferstate_scissor_test_disable);
        GLEMUScissorTestCommand* command = AddCommand(GLEMUScissorTestCommand);
        command->is_enable = false;
    }
    
    void ScissorTest(int x,int y,int width,int height)
    {
        AddHeader(glemu_bufferstate_scissor_rect_change);
        GLEMUScissorRectCommand* command = AddCommand(GLEMUScissorRectCommand);
        ScissorRect s_rect = {};
        s_rect.x = x;
        s_rect.y = y;
        s_rect.width = width;
        s_rect.height = height;
        command->s_rect = s_rect;
    }
    
    void ScissorTest(float4 rect)
    {
        ScissorTest(rect.x(),rect.y(),rect.z(),rect.w());
    }
    
    void Viewport(int x, int y, int width, int height)
    {
        AddHeader(glemu_bufferstate_viewport_change);
        GLEMUViewportChangeCommand* command = AddCommand(GLEMUViewportChangeCommand);
        command->viewport = float4(x,y,width,height);
    }
    void Viewport(float4 vp)
    {
        Viewport(vp.x(),vp.y(),vp.z(),vp.w());
    }
    
    void BindFrameBufferStart(GLTexture texture)
    {
        Texture texture_for_framebuffer = texture.texture;
        if(texture_for_framebuffer.state != nullptr)
        {
            AddHeader(glemu_bufferstate_start);
            GLEMUFramebufferStart* command = AddCommand(GLEMUFramebufferStart);
            command->texture = texture_for_framebuffer;
        }
    }
    
    void BindFrameBufferEnd()
    { 
        AddHeader(glemu_bufferstate_end);
        GLEMUFramebufferEnd* command = AddCommand(GLEMUFramebufferEnd);       
    }
    
    void EnableStencilTest()
    {
        AddHeader(glemu_bufferstate_stencil_enable);
        GLEMUStencilStateCommand* command = AddCommand(GLEMUStencilStateCommand);
        command->is_enable = true;        
    }
    
    void DisableStencilTest()
    {
        AddHeader(glemu_bufferstate_stencil_disable);
        GLEMUStencilStateCommand* command = AddCommand(GLEMUStencilStateCommand);
        command->is_enable = false;
    }
    
    void StencilMask(uint32_t mask)
    {
        AddHeader(glemu_bufferstate_stencil_mask);
        GLEMUStencilMaskCommand* command = AddCommand(GLEMUStencilMaskCommand);
        command->write_mask_value = mask;
        ds.frontFaceStencil.write_mask = mask;
        ds.backFaceStencil.write_mask = mask;
    }
    
    void StencilMaskSep(uint32_t front_or_back,uint32_t mask)
    {
        AddHeader(glemu_bufferstate_stencil_mask_sep);
        GLEMUStencilMaskSepCommand* command = AddCommand(GLEMUStencilMaskSepCommand);
        command->write_mask_value = mask;
        command->front_or_back = front_or_back;
        
        if(front_or_back)
        {
            ds.frontFaceStencil.write_mask = mask;
        }
        else
        {
            ds.backFaceStencil.write_mask = mask;                                
        }
    }
    
    void StencilFunc(CompareFunc func,uint32_t ref,uint32_t mask)
    {
        AddHeader(glemu_bufferstate_stencil_func);
        GLEMUStencilFunCommand* command = AddCommand(GLEMUStencilFunCommand);
        command->compareFunction = func;
        command->mask_value = ref;
        command->write_mask_value = mask;
        
        ds.frontFaceStencil.stencilCompareFunction = func;
        ds.frontFaceStencil.read_mask = mask;
        ds.backFaceStencil.stencilCompareFunction = func;
        ds.backFaceStencil.read_mask = mask;
        
        current_reference_value = ref;
    }
    
    void StencilFuncSep(uint32_t front_or_back,CompareFunc func,uint32_t ref,uint32_t mask)
    {
        AddHeader(glemu_bufferstate_stencil_func_sep);
        GLEMUStencilFunSepCommand* command = AddCommand(GLEMUStencilFunSepCommand);
        command->front_or_back = front_or_back;
        command->compareFunction = func;
        command->mask_value = ref;
        command->write_mask_value = mask;
        
        if(front_or_back)
        {
            ds.frontFaceStencil.stencilCompareFunction = func;
            ds.frontFaceStencil.read_mask = mask;
        }
        else
        {
            ds.backFaceStencil.stencilCompareFunction = func;
            ds.backFaceStencil.read_mask = mask;
        }
        current_reference_value = ref;
    }
    
    void StencilOperation(StencilOp sten_fail,StencilOp dpfail,StencilOp dppass)
    {
        AddHeader(glemu_bufferstate_stencil_op);
        GLEMUStencilOpCommand* command = AddCommand(GLEMUStencilOpCommand);
        command->stencil_fail_op = sten_fail;
        command->depth_fail_op = dpfail;
        command->depth_stencil_pass_op = dppass;
        
        ds.frontFaceStencil.stencilFailureOperation = sten_fail;
        ds.frontFaceStencil.depthFailureOperation = dpfail;
        ds.frontFaceStencil.depthStencilPassOperation = dppass;
        
        ds.backFaceStencil.stencilFailureOperation = sten_fail;
        ds.backFaceStencil.depthFailureOperation = dpfail;
        ds.backFaceStencil.depthStencilPassOperation = dppass;
    }
    
    void StencilOperationSep(uint32_t front_or_back,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass)
    {
        AddHeader(glemu_bufferstate_stencil_op_sep);
        GLEMUStencilOpSepCommand* command = AddCommand(GLEMUStencilOpSepCommand);
        command->front_or_back = front_or_back;
        command->stencil_fail_op = sten_fail;
        command->depth_fail_op = dpfail;
        command->depth_stencil_pass_op = dppass;
        
        if(front_or_back)
        {
            ds.frontFaceStencil.stencilFailureOperation = sten_fail;
            ds.frontFaceStencil.depthFailureOperation = dpfail;
            ds.frontFaceStencil.depthStencilPassOperation = dppass;
        }
        else
        {
            ds.backFaceStencil.stencilFailureOperation = sten_fail;
            ds.backFaceStencil.depthFailureOperation = dpfail;
            ds.backFaceStencil.depthStencilPassOperation = dppass;                                
        }
    }
    
    void StencilFuncAndOp(CompareFunc func,uint32_t ref,uint32_t mask,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass)
    {
        AddHeader(glemu_bufferstate_stencil_func_and_op);
        GLEMUStencilFuncAndOpCommand* command = AddCommand(GLEMUStencilFuncAndOpCommand);
        command->compareFunction = func;
        command->write_mask_value = mask;
        command->stencil_fail_op = sten_fail;
        command->depth_fail_op = dpfail;
        command->depth_stencil_pass_op = dppass;
        command->mask_value = ref;        
        
        ds.frontFaceStencil.stencilCompareFunction = func;
        ds.frontFaceStencil.read_mask = mask;
        ds.backFaceStencil.stencilCompareFunction = func;
        ds.backFaceStencil.read_mask = mask;
        
        ds.frontFaceStencil.stencilFailureOperation = sten_fail;
        ds.frontFaceStencil.depthFailureOperation = dpfail;
        ds.frontFaceStencil.depthStencilPassOperation = dppass;
        
        ds.backFaceStencil.stencilFailureOperation = sten_fail;
        ds.backFaceStencil.depthFailureOperation = dpfail;
        ds.backFaceStencil.depthStencilPassOperation = dppass;
        
        current_reference_value = ref;
    }
    
    void StencilFuncAndOpSep(uint32_t front_or_back,CompareFunc func,uint32_t ref,uint32_t mask,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass)
    {
        AddHeader(glemu_bufferstate_stencil_func_and_op_sep);
        GLEMUStencilFuncAndOpSepCommand* command = AddCommand(GLEMUStencilFuncAndOpSepCommand);
        command->front_or_back = front_or_back;
        command->compareFunction = func;
        command->write_mask_value = mask;
        command->stencil_fail_op = sten_fail;
        command->depth_fail_op = dpfail;
        command->depth_stencil_pass_op = dppass;
        command->mask_value = ref;        
        
        if(front_or_back)
        {
            ds.frontFaceStencil.stencilCompareFunction = func;
            ds.frontFaceStencil.read_mask = mask;
            
            ds.frontFaceStencil.stencilFailureOperation = sten_fail;
            ds.frontFaceStencil.depthFailureOperation = dpfail;
            ds.frontFaceStencil.depthStencilPassOperation = dppass;
        }
        else
        {
            ds.backFaceStencil.stencilFailureOperation = sten_fail;
            ds.backFaceStencil.depthFailureOperation = dpfail;
            ds.backFaceStencil.depthStencilPassOperation = dppass;                                
            ds.backFaceStencil.stencilCompareFunction = func;
            ds.backFaceStencil.read_mask = mask;
        }
        current_reference_value = ref;
    }
    
    void ClearBuffer(uint32_t buffer_bits)
    {
        AddHeader(glemu_bufferstate_clear_start);
        GLEMUClearBufferCommand* start_command = AddCommand(GLEMUClearBufferCommand);
		start_command->write_mask_value = buffer_bits;
		start_command->is_start = true;
        
        AddHeader(glemu_bufferstate_clear_end);
        GLEMUClearBufferCommand* end_command = AddCommand(GLEMUClearBufferCommand);        
		end_command->is_start = false;
    }
    
    void ClearStencil(uint32_t value)
    {
        AddHeader(glemu_bufferstate_clear_stencil_value);
        GLEMUClearStencilCommand* command = AddCommand(GLEMUClearStencilCommand);        
        command->write_mask_value = value;
    }
    
    void ClearColor(float4 value)
    {
        AddHeader(glemu_bufferstate_clear_color_value);
        GLEMUClearColorCommand* command = AddCommand(GLEMUClearColorCommand);                
        command->clear_color = value;
    }
    
    void ClearColorAndStencil(float4 color,uint32_t stencil)
    {
        AddHeader(glemu_bufferstate_clear_color_and_stencil_value);
        GLEMUClearColorAndStencilCommand* command = AddCommand(GLEMUClearColorAndStencilCommand);                
        command->clear_color = color;
        command->write_mask_value = stencil;            
    }
    
    //Debug calls
    void AddDebugSignPost(char* str)
    {
        AddHeader(glemu_bufferstate_debug_signpost);
        GLEMUAddDebugSignPostCommand* command = AddCommand(GLEMUAddDebugSignPostCommand);                
        command->string = str;
    }
    
    //NOTE(Ray):We now will reference the draw table when we add a draw and when we dispatch one
    //We grab an table entry to get all the relevant uniform data for draw call and all the
    //other info like texture bindings etc... for now at the time of writing this comment we only need to
    //keep uniforms and maybe texture bindings to complete the current project.
    //TODO(Ray):These will clean up nice once we get uniform variable support
    void DrawArrays(uint32_t current_count,uint32_t unit_size,PrimitiveType primitive_type)
    {
        Assert(current_count != 0);
        AddHeader(glemu_bufferstate_draw_arrays);
        GLEMUDrawArraysCommand* command = AddCommand(GLEMUDrawArraysCommand);                
        command->is_from_to = true;
        command->is_primitive_triangles = false;
        command->primitive_type = primitive_type;
        
        GLProgramKey key = {(uint64_t)current_program.shader.vs_object,(uint64_t)current_program.shader.ps_object};            
        GLProgram* p = OpenGLEmu::GetProgramPtr(key);
        
        uint32_t v_buffer_binding = p->last_fragment_buffer_binding;
        uint32_t f_buffer_binding = p->last_vertex_buffer_binding;
        
        uint32_t v_data_index     = p->last_vertex_data_index;
        uint32_t f_data_index     = p->last_fragment_data_index;
        
        BufferOffsetResult v_uni_bind_result = OpenGLEmu::GetUniformAtBinding(v_buffer_binding,v_data_index);
        BufferOffsetResult f_uni_bind_result = OpenGLEmu::GetUniformAtBinding(f_buffer_binding,f_data_index);
        BufferOffsetResult tex_binds = {};
        command->uniform_table_index = OpenGLEmu::AddDrawCallEntry(v_uni_bind_result,f_uni_bind_result,tex_binds);
        command->buffer_range = range_of_current_bound_buffers;
        command->texture_buffer_range = range_of_current_bound_frag_textures;
        command->current_count = current_count;
        EndDraw(unit_size);
        
#if GLEMU_DEBUG
        VerifyCommandBuffer(glemu_bufferstate_draw_arrays);
#endif
        
    }
    
    void DrawArrayPrimitives(uint32_t current_count,uint32_t unit_size)
    {
        Assert(current_count != 0);
        AddHeader(glemu_bufferstate_draw_arrays);
        GLEMUDrawArraysCommand* command = AddCommand(GLEMUDrawArraysCommand);                
        command->is_from_to = true;
        command->is_primitive_triangles = true;
        command->primitive_type = primitive_type_triangle;
        
        GLProgramKey key = {(uint64_t)current_program.shader.vs_object,(uint64_t)current_program.shader.ps_object};
        GLProgram p = OpenGLEmu::GetProgram(key);
        
        uint32_t v_buffer_binding = p.last_fragment_buffer_binding;
        uint32_t f_buffer_binding = p.last_vertex_buffer_binding;
        
        uint32_t v_data_index     = p.last_vertex_data_index;
        uint32_t f_data_index     = p.last_fragment_data_index;
        
        BufferOffsetResult v_uni_bind_result = OpenGLEmu::GetUniformAtBinding(v_buffer_binding,v_data_index);
        BufferOffsetResult f_uni_bind_result = OpenGLEmu::GetUniformAtBinding(f_buffer_binding,f_data_index);
        BufferOffsetResult tex_binds = {};
        command->uniform_table_index = OpenGLEmu::AddDrawCallEntry(v_uni_bind_result,f_uni_bind_result,tex_binds);
        command->buffer_range = range_of_current_bound_buffers;
        command->texture_buffer_range = range_of_current_bound_frag_textures;
        command->current_count = current_count;
        EndDraw(unit_size);
        
#if GLEMU_DEBUG
        VerifyCommandBuffer(glemu_bufferstate_draw_arrays);
#endif
        
    }
    
#define Pop(ptr,type) (type*)Pop_(ptr,sizeof(type));ptr = (uint8_t*)ptr + (sizeof(type));
    static inline void*  Pop_(void* ptr,uint32_t size)
    {
        return ptr;
    }
    
    void Execute(void* pass_in_c_buffer)
    {
        void* c_buffer = nullptr;
        if(pass_in_c_buffer)
        {
            c_buffer = pass_in_c_buffer;
        }
        else
        {
            c_buffer = RenderEncoderCode::CommandBuffer();
        }
        
        //define start values for the gl render state
        float2 scaled_dim = RendererCode::dim;// * RendererCode::display_scale_factor;
        ScissorRect default_s_rect = {0,0,(int)scaled_dim.x(),(int)scaled_dim.y()};

        ScissorRect s_rect_value = default_s_rect;
        Drawable current_drawable = RenderEncoderCode::GetDefaultDrawableFromView();

        DepthStencilDescription current_depth_desc = OpenGLEmu::GetDefaultDepthStencilDescriptor();
        GLProgram current_program = {};
        GLProgram default_program = {};
        TripleGPUBuffer* default_bind_buffer = OpenGLEmu::GetBufferAtBinding(0);
        
        uint32_t render_encoder_count = 0;
        float4 current_clear_color = float4(0.0f);
        if(current_drawable.state)
        {
            //Set default in_params for passes
            MatrixPassInParams in_params = {};
            in_params.s_rect = default_s_rect;
            in_params.current_drawable = current_drawable;
            in_params.viewport = float4(0,0,current_drawable.texture.descriptor.width,current_drawable.texture.descriptor.height);
            in_params.vertexbuffer = default_bind_buffer;
            in_params.pipeline_state = default_pipeline_state;
            
            RenderPipelineState prev_pso = {};
            ScissorRect prev_s_rect = {};
            RenderPassDescriptor current_pass_desc = default_render_pass_descriptor;
            RenderPassDescriptor prev_pass_desc = {};
            RenderPassDescriptor last_set_pass_desc = {};
            Texture render_texture = current_drawable.texture;
            Texture current_render_texture = render_texture;
            
            bool init_params = false;
            bool depth_state_change = false;
            
            u32 current_command_index = 0;
            void* at = command_list.buffer.base;

#if METALIZER_DEBUG_OUTPUT
            PlatformOutput(debug_out_general, "GLEMU EXECTING COMMANDS COUNT: %d -- \n",command_list.count);
#endif

            while (current_command_index < command_list.count)
            {
                GLEMUCommandHeader* header = (GLEMUCommandHeader*)at;
                at = (uint8_t*)at + sizeof(GLEMUCommandHeader);
                GLEMUBufferState command_type = header->type;
                ++current_command_index;


                if(command_type == glemu_bufferstate_debug_signpost)
                {
                    GLEMUAddDebugSignPostCommand* command = Pop(at,GLEMUAddDebugSignPostCommand);                            
#ifdef METALIZER_INSERT_DEBUGSIGNPOST
                    RenderDebug::InsertDebugSignPost(in_params.re,command->string);
#endif

#if METALIZER_DEBUG_OUTPUT
                                        PlatformOutput(debug_out_general,command->string);
                                        PlatformOutput(debug_out_general,"\n");
#endif                                                
                    continue;    
                }
                
                if(!init_params)
                {
                    RenderEncoderCode::SetRenderPassColorAttachmentTexture(&render_texture,&current_pass_desc,0);
                    RenderEncoderCode::SetRenderPassColorAttachmentDescriptor(&current_pass_desc,0);
                    RendererCode::SetRenderPassDescriptor(&current_pass_desc);
                    current_render_texture = render_texture;                            
                    render_encoder_count++;

                    RenderCommandEncoder re = RenderEncoderCode::RenderCommandEncoderWithDescriptor(c_buffer,&current_pass_desc);
                    RenderEncoderCode::SetFrontFaceWinding(&re,winding_order_counter_clockwise);
                    in_params.re = re;
                    last_set_pass_desc = current_pass_desc;
#if METALIZER_DEBUG_OUTPUT
                    PlatformOutput(debug_out_general,"Setting last set_pass_desc\n");
#endif
                    //Verify and set pipeline states attachments to the same as our current renderpass
                    //Do depth and stencil only for now bu tlater we want to ensure that our color attachments match as well.
                    if(in_params.pipeline_state.desc.depthAttachmentPixelFormat != current_pass_desc.depth_attachment.description.texture.descriptor.pixelFormat)
                    {
                        depth_state_change = false;
                        RenderPipelineStateDesc pd = in_params.pipeline_state.desc;
                        pd.depthAttachmentPixelFormat = current_pass_desc.depth_attachment.description.texture.descriptor.pixelFormat;
                        pd.stencilAttachmentPixelFormat = current_pass_desc.depth_attachment.description.texture.descriptor.pixelFormat;
                        
                        //if its and someone tried to use it should just set it as a non texture
                        //and sync everything  just dont let client use invalid textures as color attachments
                        //put out a warning or silently fail?
                        if(current_pass_desc.depth_attachment.description.texture.state == nullptr)
                        {
                            pd.depthAttachmentPixelFormat = PixelFormatInvalid;
                            pd.stencilAttachmentPixelFormat = PixelFormatInvalid;
                        }
                        
#if METALIZER_DEBUG_OUTPUT
                        PlatformOutput(debug_out_general,"NewPIpelineState::pd\n");
#endif
                        
                        RenderPipelineState next_pso = RenderEncoderCode::NewRenderPipelineStateWithDescriptor(pd);
                        Assert(next_pso.desc.fragment_function);
                        Assert(next_pso.desc.sample_count == 1);
                        Assert(next_pso.desc.vertex_function);
                        Assert(next_pso.state);
                        
                        RenderEncoderCode::SetRenderPipelineState(&in_params.re,next_pso.state);
                        prev_pso = in_params.pipeline_state;
                        in_params.pipeline_state = next_pso;
                    }
                    
                    //NOTE(Ray):If we are scissor enable we need to clamp the scissor rect to the rendertarget surface
                    if(in_params.is_s_rect)
                    {
                        ScissorRect temp_rect_value = in_params.s_rect;
                        //NOTE(Ray):GL is from bottom left we are top left converting y cooridinates to match
                        temp_rect_value.y = current_render_texture.descriptor.height - (temp_rect_value.height + temp_rect_value.y);
//                        temp_rect_value.y = default_s_rect.height - temp_rect_value.y;
                        //NOTE(Ray):Not allowed to specify a value outside of the current renderpass width in metal.
                        int diffw = (int)current_render_texture.descriptor.width - (temp_rect_value.x + temp_rect_value.width);
                        int diffh = (int)current_render_texture.descriptor.height - (temp_rect_value.y + temp_rect_value.height);
                        if(diffw < 0)
                            temp_rect_value.width += diffw;
                        if(diffh < 0)
                            temp_rect_value.height += diffh;
                        
                        temp_rect_value.x = clamp(temp_rect_value.x,0,current_render_texture.descriptor.width);
                        temp_rect_value.y = clamp(temp_rect_value.y,0,current_render_texture.descriptor.height);
                        temp_rect_value.width = clamp(temp_rect_value.width,0,current_render_texture.descriptor.width - temp_rect_value.x);
                        temp_rect_value.height = clamp(temp_rect_value.height,0,current_render_texture.descriptor.height - temp_rect_value.y);
                        
                        //in_params.s_rect = temp_rect_value;
                        RenderEncoderCode::SetScissorRect(&in_params.re, temp_rect_value);
                    }
                    init_params = true;
                }
               
                if(depth_state_change == true)
                {
                    depth_state_change = false;
                    RenderPipelineStateDesc pd = in_params.pipeline_state.desc;
                    pd.depthAttachmentPixelFormat = current_pass_desc.depth_attachment.description.texture.descriptor.pixelFormat;
                    pd.stencilAttachmentPixelFormat = current_pass_desc.depth_attachment.description.texture.descriptor.pixelFormat;
                    
                    //if its and someone tried to use it should just set it as a non texture
                    //and sync everything  just dont let client use invalid textures as color attachments
                    //put out a warning or silently fail?
                    if(current_pass_desc.depth_attachment.description.texture.state == nullptr)
                    {
                        pd.depthAttachmentPixelFormat = PixelFormatInvalid;
                        pd.stencilAttachmentPixelFormat = PixelFormatInvalid;
                    }
                    
#if METALIZER_DEBUG_OUTPUT
                    PlatformOutput(debug_out_general,"NewPIpelineState::pd\n");
#endif
                    
                    RenderPipelineState next_pso = RenderEncoderCode::NewRenderPipelineStateWithDescriptor(pd);
                    Assert(next_pso.desc.fragment_function);
                    Assert(next_pso.desc.sample_count == 1);
                    Assert(next_pso.desc.vertex_function);
                    Assert(next_pso.state);
                    
                    RenderEncoderCode::SetRenderPipelineState(&in_params.re,next_pso.state);
                    prev_pso = in_params.pipeline_state;
                    in_params.pipeline_state = next_pso;
                    
                }
                
                if(command_type == glemu_bufferstate_start)
                {
                    GLEMUFramebufferStart* command = Pop(at,GLEMUFramebufferStart);//(GLEMUFramebufferStart)at;
                    if(command->texture.state != current_render_texture.state)
                    {
                        render_texture = command->texture;

                        prev_pass_desc = last_set_pass_desc;
                        
                        RenderPassDescriptor temp_desc = last_set_pass_desc;
                        //depth attachment description
                        Texture no_tex = {};
                        temp_desc.depth_attachment.description.texture       = no_tex;
                        temp_desc.depth_attachment.description.loadAction    = LoadActionDontCare;
                        temp_desc.depth_attachment.description.storeAction   = StoreActionDontCare;
                        
                        temp_desc.stencil_attachment.description.texture       = no_tex;
                        temp_desc.stencil_attachment.description.loadAction    = LoadActionDontCare;
                        temp_desc.stencil_attachment.description.storeAction   = StoreActionDontCare;
                        
                        RendererCode::SetRenderPassDescriptor(&temp_desc);
                        
                        current_pass_desc = temp_desc;
#if METALIZER_DEBUG_OUTPUT
                                        PlatformOutput(debug_out_general, "GLEMU BUFFERSTATE START\n");
#endif                                                
                        RenderEncoderCode::EndEncoding(&in_params.re);
                        init_params = false;                                
                    }
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_end)
                {
                    GLEMUFramebufferEnd* command = Pop(at,GLEMUFramebufferEnd);
                    if(current_drawable.state != current_render_texture.state)
                    {
                        Assert(prev_pass_desc.stencil_attachment.description.texture.state);
                        current_pass_desc = prev_pass_desc;
                        Assert(current_pass_desc.stencil_attachment.description.texture.state);
                        render_texture = current_drawable.texture;
                        RenderEncoderCode::EndEncoding(&in_params.re);
                        
#if METALIZER_DEBUG_OUTPUT
                        PlatformOutput(debug_out_general,"Framebuffer_framebuffer_end\n");
#endif

                        init_params = false;                                
                    }
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_clear_start)
                {
                    GLEMUClearBufferCommand* command = Pop(at,GLEMUClearBufferCommand);                            
                    //Get buffer bits and than set the attachments to the state needed.
                    //Than after the clear set them back to defaults
                    if(command->write_mask_value & (1 << 1))
                    {
                        RenderPassColorAttachmentDescriptor* ca = RenderEncoderCode::GetRenderPassColorAttachment(&current_pass_desc,0);
                        ca->description.loadAction = LoadActionClear;
                        ca->clear_color = current_clear_color;
                    }
                    
                    //TODO(Ray):Need to validate they  have actual stencil and depth textures attached if pipeline
                    //has pixel format. 
                    if(command->write_mask_value & (1 << 2))
                    {
                        current_pass_desc.depth_attachment.description.loadAction = LoadActionClear;                                
                    }
                    if(command->write_mask_value & (1 << 3))
                    {
                        current_pass_desc.stencil_attachment.description.loadAction = LoadActionClear;                                
                    }
                    
                    RendererCode::SetRenderPassDescriptor(&current_pass_desc);
                    RenderEncoderCode::EndEncoding(&in_params.re);
                    init_params = false;

#if METALIZER_DEBUG_OUTPUT
                                        PlatformOutput(debug_out_general, "GLEMU CLEAR START\n");
#endif
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_clear_end)
                {
                    GLEMUClearBufferCommand* command = Pop(at,GLEMUClearBufferCommand);
                    
                    //for the time being we are always in load to better emulate what opengl does.
                    RenderPassColorAttachmentDescriptor* ca = RenderEncoderCode::GetRenderPassColorAttachment(&current_pass_desc,0);
                    ca->description.loadAction = LoadActionLoad;
                    current_pass_desc.depth_attachment.description.loadAction = LoadActionLoad;                                
                    current_pass_desc.stencil_attachment.description.loadAction = LoadActionLoad;
                    RendererCode::SetRenderPassDescriptor(&current_pass_desc);
                    RenderEncoderCode::EndEncoding(&in_params.re);
                    init_params = false;

#if METALIZER_DEBUG_OUTPUT
                                        PlatformOutput(debug_out_general, "GLEMU CLEAR END \n");
#endif
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_clear_stencil_value)
                {
                    GLEMUClearStencilCommand* command = Pop(at,GLEMUClearStencilCommand);
                    Assert(false);
#if METALIZER_DEBUG_OUTPUT
                                        PlatformOutput(debug_out_general, "GLEMU CLEAR stencil' not implemented' \n");
#endif
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_clear_color_value)
                {
                    GLEMUClearColorCommand* command = Pop(at,GLEMUClearColorCommand);                            
                    current_clear_color = command->clear_color;
#if METALIZER_DEBUG_OUTPUT
                                        PlatformOutput(debug_out_general, "GLEMU CLEAR Color  \n");
#endif
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_clear_color_and_stencil_value)
                {
                    GLEMUClearColorAndStencilCommand* command = Pop(at,GLEMUClearColorAndStencilCommand);
//Not properly implemented
                    Assert(false);
#if METALIZER_DEBUG_OUTPUT
                                        PlatformOutput(debug_out_general, "GLEMU CLEAR Color and stencil command \n");
#endif
                    current_clear_color = command->clear_color;
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_viewport_change)
                {
                    GLEMUViewportChangeCommand* command = Pop(at,GLEMUViewportChangeCommand);
                    
#if METALIZER_DEBUG_OUTPUT
                    PlatformOutput(debug_out_general, "Viewport l:%f t:%f b:%f r:%f .\n",command->viewport.x(),command->viewport.y(),command->viewport.z(),command->viewport.w());
#endif
                    
                    //TODO(Ray):We need to add some checks here to keep viewport in surface bounds.
                    in_params.viewport = command->viewport;
                    float4 vp = in_params.viewport;
                    RenderEncoderCode::SetViewport(&in_params.re,vp.x(),vp.y(),vp.z(),vp.w(),0.0f,1.0f);                            
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_blend_change)
                {
                    GLEMUBlendCommand* command = Pop(at,GLEMUBlendCommand);
                    BlendFactor source = command->sourceRGBBlendFactor;
                    BlendFactor dest = command->destinationRGBBlendFactor;
                    
                    RenderPipelineStateDesc pd = in_params.pipeline_state.desc;
                    RenderPipelineColorAttachmentDescriptor cad = in_params.pipeline_state.desc.color_attachments.i[0];
                    //                            if(cad.sourceRGBBlendFactor != source || cad.destinationRGBBlendFactor != dest)
                    {
                        
                        Assert((int)source >= 0);
                        Assert((int)dest >= 0);
                        
                        cad.sourceRGBBlendFactor = source;
                        cad.sourceAlphaBlendFactor = source;
                        cad.destinationRGBBlendFactor = dest;
                        cad.destinationAlphaBlendFactor = dest;
                        pd.color_attachments.i[0] = cad;
                        
                        RenderPipelineState next_pso = RenderEncoderCode::NewRenderPipelineStateWithDescriptor(pd);
                        Assert(next_pso.desc.fragment_function);
                        Assert(next_pso.desc.sample_count == 1);
                        Assert(next_pso.desc.vertex_function);
                        Assert(next_pso.state);
#if METALIZER_DEBUG_OUTPUT
                        PlatformOutput(debug_out_general,"Framebuffer_blend_change::New Pipeline State\n");
#endif
                        in_params.pipeline_state = next_pso;
                        RenderEncoderCode::SetRenderPipelineState(&in_params.re,in_params.pipeline_state.state);
                    }
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_shader_program_change)
                {
                    GLEMUUseProgramCommand* command = Pop(at,GLEMUUseProgramCommand);

//TODO(Ray):Give programs IDS and make sure we dont need to switch here if its the same program
//                    if(command->program.id != current_program.id)
                    {
                        GLProgram new_program = command->program;
                        if(!new_program.shader.vs_object || !new_program.shader.ps_object)continue;
                        RenderPipelineStateDesc pd = in_params.pipeline_state.desc;
                        pd.vertex_function = new_program.shader.vs_object;
                        pd.fragment_function = new_program.shader.ps_object;
                        RenderEncoderCode::SetVertexDescriptor(&pd, &new_program.vd);
                        
                        RenderPipelineState next_pso = RenderEncoderCode::NewRenderPipelineStateWithDescriptor(pd);
                        
                        Assert(next_pso.desc.depthAttachmentPixelFormat == pd.depthAttachmentPixelFormat);
                        Assert(next_pso.desc.stencilAttachmentPixelFormat == pd.stencilAttachmentPixelFormat);
                        Assert(next_pso.desc.fragment_function);
                        Assert(next_pso.desc.sample_count == 1);
                        Assert(next_pso.desc.vertex_function);
                        Assert(next_pso.state);
#if METALIZER_DEBUG_OUTPUT                                
                        PlatformOutput(debug_out_program_change,"Framebuffer_shader_program_change::New Pipeline State\n");
#endif
                        current_program = new_program;
                        in_params.pipeline_state = next_pso;
                        RenderEncoderCode::SetRenderPipelineState(&in_params.re,in_params.pipeline_state.state);
                    }
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_scissor_test_enable)
                {
                    GLEMUScissorTestCommand* command = Pop(at,GLEMUScissorTestCommand);
                    in_params.is_s_rect = true;

#if METALIZER_DEBUG_OUTPUT                                
                        PlatformOutput(debug_out_general,"Framebuffer_scissor test enable\n");
#endif
                    RenderEncoderCode::SetScissorRect(&in_params.re, in_params.s_rect);
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_scissor_test_disable)
                {
                    GLEMUScissorTestCommand* command = Pop(at,GLEMUScissorTestCommand);
                    in_params.is_s_rect = false;
                    ScissorRect new_s_rect = {};
                    
//                    new_s_rect.width = prev_s_rect.width;//current_render_texture.descriptor.width;
//                    new_s_rect.height = prev_s_rect.height;//current_render_texture.descriptor.height;
                    new_s_rect.width = current_render_texture.descriptor.width;
                    new_s_rect.height = current_render_texture.descriptor.height;                    
                    new_s_rect.x = 0;
                    new_s_rect.y = 0;

#if METALIZER_DEBUG_OUTPUT                                
                        PlatformOutput(debug_out_general,"Framebuffer_scissor test disable\n");
#endif
                    RenderEncoderCode::SetScissorRect(&in_params.re, new_s_rect);
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_scissor_rect_change)
                {
                    GLEMUScissorRectCommand* command = Pop(at,GLEMUScissorRectCommand);
                    ScissorRect temp_rect_value = command->s_rect;
                    prev_s_rect = in_params.s_rect;
                    //NOTE(Ray):GL is from bottom left we are top left converting y cooridinates to match
                    temp_rect_value.y = current_render_texture.descriptor.height - (temp_rect_value.height + temp_rect_value.y);
                    //NOTE(Ray):Not allowed to specify a value outside of the current renderpass width in metal.
                    int diffw = (int)current_render_texture.descriptor.width - (temp_rect_value.x + temp_rect_value.width);
                    int diffh = (int)current_render_texture.descriptor.height - (temp_rect_value.y + temp_rect_value.height);
                    if(diffw < 0)
                        temp_rect_value.width += diffw;
                    if(diffh < 0)
                        temp_rect_value.height += diffh;
                    
                    temp_rect_value.x = clamp(temp_rect_value.x,0,current_render_texture.descriptor.width);
                    temp_rect_value.y = clamp(temp_rect_value.y,0,current_render_texture.descriptor.height);
                    temp_rect_value.width = clamp(temp_rect_value.width,0,current_render_texture.descriptor.width - temp_rect_value.x);
                    temp_rect_value.height = clamp(temp_rect_value.height,0,current_render_texture.descriptor.height - temp_rect_value.y);
                    
#if METALIZER_DEBUG_OUTPUT
                    PlatformOutput(debug_out_general,"Scissor Rect Change\n");
#endif
                    s_rect_value = temp_rect_value;
                    in_params.s_rect = command->s_rect;
                    if(in_params.is_s_rect)
                    {
                        RenderEncoderCode::SetScissorRect(&in_params.re, temp_rect_value);
                    }
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_stencil_enable)
                {
                    GLEMUStencilStateCommand* command = Pop(at,GLEMUStencilStateCommand);
                    
#ifdef METALIZER_INSERT_DEBUGSIGNPOST
                    char* string = "Stencil Enabled:";
                    RenderDebug::InsertDebugSignPost(in_params.re,string);
#endif
                    //                            RenderDebug::PushDebugGroup(in_params.re,string);
#if METALIZER_DEBUG_OUTPUT
                    PlatformOutput(debug_out_general,"Framebuffer_stencil_enable\n");
#endif
                    //NOTE("Must havea  stencil buffer attached here");
                    //The stencil should be the same size as the render target.
                    //if not fragments out of the bounds of the stencil surface will get clipped.
                    //Ensure we have a valid sized stencil buffer for the current render texture 
                    current_depth_desc.backFaceStencil.enabled = true;
                    current_depth_desc.frontFaceStencil.enabled = true;
                    DepthStencilState state = OpenGLEmu::GetOrCreateDepthStencilState(current_depth_desc);
                    RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
                    //TODO(Ray):ASSERT AFTER EVERY SET DEPTHSTENCILSTATE TO ENSURE WE GOT A VALID STATE BACKK                            
                    if(!last_set_pass_desc.stencil_attachment.description.texture.state)
                    {
                        //                                Assert(current_pass_desc.stencil_attachment.description.texture.state);
                        //Assert(prev_pass_desc.stencil_attachment.description.texture.state);
                        //NOTE(Ray):Since we need a valid texture set for the renderpass end encoding and
                        //start a new encoder with a valid texture set on the render pass descriptor.
                        RendererCode::SetRenderPassDescriptor(&current_pass_desc);
                        RenderEncoderCode::EndEncoding(&in_params.re);
#if METALIZER_DEBUG_OUTPUT
                        PlatformOutput(debug_out_general,"End Encoding Setting renderpassdesc to have texture \n");
#endif
                        init_params = false;                                                          
                    }
                    is_stencil_enabled = true;
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_stencil_disable)
                {
                    GLEMUStencilStateCommand* command = Pop(at,GLEMUStencilStateCommand);
                    current_depth_desc.frontFaceStencil.enabled = false;
                    current_depth_desc.backFaceStencil.enabled = false;
                    DepthStencilState state = OpenGLEmu::GetOrCreateDepthStencilState(current_depth_desc);
                    RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
                    RenderDebug::PopDebugGroup(in_params.re);
#ifdef METALIZER_INSERT_DEBUGSIGNPOST
                    char* string = "Stencil Disabled:";
                    RenderDebug::InsertDebugSignPost(in_params.re,string);
#endif
                    is_stencil_enabled = false;
                    depth_state_change = true;                    
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_stencil_mask)
                {
                    GLEMUStencilMaskCommand* command = Pop(at,GLEMUStencilMaskCommand);
                    current_depth_desc.frontFaceStencil.write_mask = command->write_mask_value;
                    current_depth_desc.backFaceStencil.write_mask = command->write_mask_value;
                    DepthStencilState state = OpenGLEmu::GetOrCreateDepthStencilState(current_depth_desc);
                    RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
#if METALIZER_DEBUG_OUTPUT                                
                        PlatformOutput(debug_out_general,"Framebuffer_stencil mask\n");
#endif

                        depth_state_change = true;
                    continue;                            
                }
                
                else if(command_type == glemu_bufferstate_stencil_mask_sep)
                {
                    GLEMUStencilMaskSepCommand* command = Pop(at,GLEMUStencilMaskSepCommand);
                    if(command->front_or_back)
                    {
                        current_depth_desc.frontFaceStencil.write_mask = command->write_mask_value;
                    }
                    else
                    {
                        current_depth_desc.backFaceStencil.write_mask = command->write_mask_value;                                
                    }
                    DepthStencilState state = OpenGLEmu::GetOrCreateDepthStencilState(current_depth_desc);
                    RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);

                    depth_state_change = true;
#if METALIZER_DEBUG_OUTPUT                                
                        PlatformOutput(debug_out_general,"Framebuffer_stencil mask sep\n");
#endif
                    continue;                            
                }
                
                else if(command_type == glemu_bufferstate_stencil_func)
                {
                    GLEMUStencilFunCommand* command = Pop(at,GLEMUStencilFunCommand);
                    current_depth_desc.frontFaceStencil.stencilCompareFunction = command->compareFunction;
                    current_depth_desc.frontFaceStencil.read_mask = command->write_mask_value;
                    current_depth_desc.backFaceStencil.stencilCompareFunction = command->compareFunction;
                    current_depth_desc.backFaceStencil.read_mask = command->write_mask_value;
                    RenderEncoderCode::SetStencilReferenceValue(in_params.re,command->mask_value);
                    DepthStencilState state = OpenGLEmu::GetOrCreateDepthStencilState(current_depth_desc);
                    RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);

                    depth_state_change = true;                        
#if METALIZER_DEBUG_OUTPUT                                
                        PlatformOutput(debug_out_general,"Framebuffer_stencil func\n");
#endif


                    continue;
                }
                
                else if(command_type == glemu_bufferstate_stencil_func_sep)
                {
                    GLEMUStencilFunSepCommand* command = Pop(at,GLEMUStencilFunSepCommand);
                    if(command->front_or_back)
                    {
                        current_depth_desc.frontFaceStencil.stencilCompareFunction = command->compareFunction;
                        current_depth_desc.frontFaceStencil.read_mask = command->write_mask_value;
                    }
                    else
                    {
                        current_depth_desc.backFaceStencil.stencilCompareFunction = command->compareFunction;
                        current_depth_desc.backFaceStencil.read_mask = command->write_mask_value;
                    }
                    RenderEncoderCode::SetStencilReferenceValue(in_params.re,command->mask_value);
                    DepthStencilState state = OpenGLEmu::GetOrCreateDepthStencilState(current_depth_desc);
                    RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);

                    depth_state_change = true;                                            
#if METALIZER_DEBUG_OUTPUT                                
                        PlatformOutput(debug_out_general,"Framebuffer_stencil func sep\n");
#endif
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_stencil_op)
                {
                    GLEMUStencilOpCommand* command = Pop(at,GLEMUStencilOpCommand);
                    current_depth_desc.frontFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                    current_depth_desc.frontFaceStencil.depthFailureOperation = command->depth_fail_op;
                    current_depth_desc.frontFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;
                    
                    current_depth_desc.backFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                    current_depth_desc.backFaceStencil.depthFailureOperation = command->depth_fail_op;
                    current_depth_desc.backFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;
                    if(is_stencil_enabled)
                    {
                        DepthStencilState state = OpenGLEmu::GetOrCreateDepthStencilState(current_depth_desc);
#if METALIZER_DEBUG_OUTPUT                                
                        PlatformOutput(debug_out_general,"Framebuffer_stencil  op\n");
#endif
                        RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);                        
                    }

                    depth_state_change = true;                        
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_stencil_op_sep)
                {
                    GLEMUStencilOpSepCommand* command = Pop(at,GLEMUStencilOpSepCommand);
                    if(command->front_or_back)
                    {
                        current_depth_desc.frontFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                        current_depth_desc.frontFaceStencil.depthFailureOperation = command->depth_fail_op;
                        current_depth_desc.frontFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;
                    }
                    else
                    {
                        current_depth_desc.backFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                        current_depth_desc.backFaceStencil.depthFailureOperation = command->depth_fail_op;
                        current_depth_desc.backFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;                                
                    }
                    DepthStencilState state = OpenGLEmu::GetOrCreateDepthStencilState(current_depth_desc);
                    RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);

                    depth_state_change = true;                        
#if METALIZER_DEBUG_OUTPUT                                
                        PlatformOutput(debug_out_general,"Framebuffer_stencil op sep\n");
#endif
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_stencil_func_and_op)
                {
                    GLEMUStencilFuncAndOpCommand* command = Pop(at,GLEMUStencilFuncAndOpCommand);
                    current_depth_desc.frontFaceStencil.stencilCompareFunction = command->compareFunction;
                    current_depth_desc.frontFaceStencil.read_mask = command->write_mask_value;
                    current_depth_desc.backFaceStencil.stencilCompareFunction = command->compareFunction;
                    current_depth_desc.backFaceStencil.read_mask = command->write_mask_value;
                    RenderEncoderCode::SetStencilReferenceValue(in_params.re,command->mask_value);
                    current_depth_desc.frontFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                    current_depth_desc.frontFaceStencil.depthFailureOperation = command->depth_fail_op;
                    current_depth_desc.frontFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;
                    
                    current_depth_desc.backFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                    current_depth_desc.backFaceStencil.depthFailureOperation = command->depth_fail_op;
                    current_depth_desc.backFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;
                    
                    DepthStencilState state = OpenGLEmu::GetOrCreateDepthStencilState(current_depth_desc);
                    RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);

                    depth_state_change = true;                        
#if METALIZER_DEBUG_OUTPUT                                
                        PlatformOutput(debug_out_general,"Framebuffer_stencil func and op\n");
#endif
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_stencil_func_and_op_sep)
                {
                    GLEMUStencilFuncAndOpSepCommand* command = Pop(at,GLEMUStencilFuncAndOpSepCommand);
                    if(command->front_or_back)
                    {
                        current_depth_desc.frontFaceStencil.stencilCompareFunction = command->compareFunction;
                        current_depth_desc.frontFaceStencil.read_mask = command->write_mask_value;
                        current_depth_desc.frontFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                        current_depth_desc.frontFaceStencil.depthFailureOperation = command->depth_fail_op;
                        current_depth_desc.frontFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;
                    }
                    else
                    {
                        current_depth_desc.backFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                        current_depth_desc.backFaceStencil.depthFailureOperation = command->depth_fail_op;
                        current_depth_desc.backFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;                                
                        current_depth_desc.backFaceStencil.stencilCompareFunction = command->compareFunction;
                        current_depth_desc.backFaceStencil.read_mask = command->write_mask_value;
                    }
                    RenderEncoderCode::SetStencilReferenceValue(in_params.re,command->mask_value);
                    DepthStencilState state = OpenGLEmu::GetOrCreateDepthStencilState(current_depth_desc);
                    RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);

                    depth_state_change = true;                        
#if METALIZER_DEBUG_OUTPUT                                
                        PlatformOutput(debug_out_general,"Framebuffer_stencil func and op sep\n");
#endif
                    continue;
                }
                
                else if(command_type == glemu_bufferstate_draw_arrays)
                {
                    GLEMUDrawArraysCommand* command = Pop(at,GLEMUDrawArraysCommand);
                    //None means a draw attempt here we check bindings if there are any and set up the data
                    //binding for the gpu along with sending any data to the gpu that needs sent.
                    //TODO(Ray):Later once this is working switch form buffer 3 to 2 and remove the three buffer
                    //and other uniforms setting in the uniform matrix excute callback.
                    //TODO(Ray):make sure we can actually use the same buffer index for the gpu set bytes here.
                    //TODO(Ray):Verifiy that draw index and for loop index i actually
                    UniformBindingTableEntry uni_entry = GetUniEntryForDrawCall(command->uniform_table_index);
                    if(uni_entry.v_size > 0)
                    {
                        RenderEncoderCode::SetVertexBytes(&in_params.re,uni_entry.v_data,uni_entry.v_size,4);
#if METALIZER_INSERT_DEBUGSIGNPOST
                        PlatformOutput(debug_out_uniforms,"UniformBinding table entry : v_size :%d :  buffer index %d \n",uni_entry.v_size,4);
#endif
                    }
                    if(uni_entry.f_size > 0)
                    {
                        RenderEncoderCode::SetFragmentBytes(&in_params.re,uni_entry.f_data,uni_entry.f_size,4);
#if METALIZER_INSERT_DEBUGSIGNPOST
                        PlatformOutput(debug_out_uniforms,"UnidformBinding table entry : f_size :%d : buffer index %d \n",uni_entry.f_size,4);
#endif
                    }
                    
                    //TExture bindings here
                    //for ever texture bind at texture index
                    //TODO(Ray):Allow for texture bindings on the vertex shader and compute functions.
                    float2 t_buffer_range = command->texture_buffer_range;
                    for(int i = t_buffer_range.x();i < t_buffer_range.y();++i)
                    {
                        FragmentShaderTextureBindingTableEntry* entry = YoyoGetVectorElement(FragmentShaderTextureBindingTableEntry,&currently_bound_frag_textures,i);
                        Assert(entry);
                        Assert(entry->texture.texture.state);
                        Assert(entry->texture.sampler.state);
                        GLTexture final_tex = entry->texture;

                        if(GLIsValidTexture(final_tex))
                        {
#ifdef METALIZER_INSERT_DEBUGSIGNPOST
                            PlatformOutput(debug_out_high,"Attempt To bind invalid texture Invalid texture  \n");
#endif
                        }
                        else
                        {
                            final_tex = default_texture;
                        }
                        
                        Assert(!final_tex.texture.is_released);

//TODO(Ray):Change this define to GLEMUDIAG
#if YOYODIAG                        
                        for(int i = 0;i < resource_managment_tables.released_textures_table.anythings.count;++i)
                        {
                            ReleasedTextureEntry* rte = YoyoGetVectorElement(ReleasedTextureEntry,&resource_managment_tables.released_textures_table.anythings,i);
                            if(rte && !rte->is_free)
                            {
                                if(AnythingCacheCode::DoesThingExist(&gl_texturecache,&rte->tex_key))
                                {
                                    GLTexture* tex = GetThingPtr(&gl_texturecache,&rte->tex_key,GLTexture);
                                    Assert(tex->id == rte->tex_key.gl_tex_id);
                                    Assert(tex->texture.state);
                                    Assert(!tex->texture.is_released);
                                    Assert(!tex->is_released);
                                    if(tex->texture.state == final_tex.texture.state)
                                    {
                                        PlatformOutput(true,"CANNOT USE TEXTURE AFTER RELEASE IS CALLED!! using tex_id %d released tex id %d \n",final_tex.id,tex->id);
                                        Assert(false);
                                    }
                                }
                            }
                        }
#endif

                        RenderEncoderCode::SetFragmentTexture(&in_params.re,&final_tex.texture,entry->tex_index);
                        //TODO(Ray):Allow for mutliple sampler bindings or perhaps none and use shader defined ones
                        //for now all textures use the sampler index 0 and they must have one defined.
                        //In other words a GLTexture at teh moment means you have a sampler with you by default.
                        RenderEncoderCode::SetFragmentSamplerState(&in_params.re,&final_tex.sampler,entry->sampler_index);
#ifdef METALIZER_DEBUG_OUTPUT
                        PlatformOutput(debug_out_high,"texture binding entry : index:%d : range index %d \n",entry->tex_index,i);
                        PlatformOutput(debug_out_high,"sampler binding entry : index:%d : range index %d \n",entry->sampler_index,i);
#endif
                    }
                    
                    //TODO(Ray):For every vertex or fragment texture binding add a binding if there is no
                    //binding for the shader at that index we should know right away we would need some introspection into the shader/
                    //at that point but we dont have that yet for current projects its not an issue.
                    
                    //TODO(Ray):If we ever need to GET shader meta data and check bindings match shader inputs.
                    
                    //Buffer bindings
                    //TODO(Ray):Allow for buffer bindings on the fragment and compute function
                    float2 buffer_range = command->buffer_range;
                    for(int i = buffer_range.x();i < buffer_range.y();++i)
                    {
                        BufferBindingTableEntry* entry = YoyoGetVectorElement(BufferBindingTableEntry,&currently_bound_buffers,i);
                        TripleGPUBuffer* t_buffer = OpenGLEmu::GetBufferAtBinding(entry->key);
                        
                        GPUBuffer buffer = t_buffer->buffer[OpenGLEmu::current_buffer_index];
                        RenderEncoderCode::SetVertexBuffer(&in_params.re,&buffer,entry->offset,entry->index);
                       // PlatformOutput(true,"buffer binding entry : index:%d : offset %d : range index %d \n",entry->index,entry->offset,i);
                    }
                    
                    RenderCommandEncoder re = in_params.re;
                    
                    uint32_t bi = current_buffer_index;
                    uint32_t current_count = command->current_count;
                    
                    GPUBuffer vertexbuffer = in_params.vertexbuffer->buffer[bi];
//                    RenderEncoderCode::DrawPrimitives(&re,command->primitive_type ,0 ,current_count);
                    RenderEncoderCode::DrawPrimitives(&re,command->primitive_type,0,current_count);                    
                }
                else
                {
                    int a = 0;
                    Assert(false);
                }
            }
            
            RenderEncoderCode::AddCompletedHandler(c_buffer,[](void* arg)
                                                   {
                                                   DispatchSemaphoreT* sema = (DispatchSemaphoreT*)arg;
                                                   RenderSynchronization::DispatchSemaphoreSignal(sema);
                                                   },&semaphore);
            
            if(init_params)
            {
                RenderEncoderCode::EndEncoding(&in_params.re);            
            }

//#ifdef GLEMU_PRESENT           
            //Tell the gpu to present the drawable that we wrote to
            RenderEncoderCode::PresentDrawable(c_buffer,current_drawable.state);
//#endif
            RenderEncoderCode::Commit(c_buffer);
            
            prev_frame_pipeline_state = in_params.pipeline_state;
        }
        
        //Render stats
        uint32_t pipelinestate_count = RenderCache::GetPipelineStateCount();
        PlatformOutput(true, "Renderpipeline states: %d\n",pipelinestate_count);
        uint32_t depth_stencil_state_count = RenderCache::GetPipelineStateCount();
        PlatformOutput(true, "DepthStencilState count: %d\n",depth_stencil_state_count);
        PlatformOutput(true, "RenderEncoder count: %d\n",render_encoder_count);
        PlatformOutput(true, "Draw count: %d\n",draw_index);
    }
};

#endif

