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
    
    DepthStencilState default_depth_stencil_state;
    DepthStencilState curent_depth_stencil_state;
    
    RenderShader default_shader;
    GLProgram default_program;
    GLProgram current_program;
    
    AnythingCache buffercache;
    //NOTE(Ray):intention was variable sized buffers for uniforms but decided on not using
    //Moving to fixed size vectors for each shader set since there should be only one set of uniforms
    //per shader. simpler easier but will keep cpubuffers around for variable size implementation
    //lat which we will sure use later for something else.
    AnythingCache cpubuffercache;
    AnythingCache programcache;
    AnythingCache uniform_buffercache;//cache of vectors each one attached to a shader
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
    uint32_t current_count;
        
    GPUBuffer buffer[3];
    MemoryArena arena[3];
    
    GPUBuffer atlas_index_buffer[3];
    MemoryArena atlas_index_arena[3];

    //??
    GPUBuffer matrix_variable_size_buffer[3];
    MemoryArena matrix_variable_size_arena[3];

    //matrix buffer add on for those who might need it.
    GPUBuffer matrix_buffer;//uniform
    MemoryArena matrix_buffer_arena;
    
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
        command_list.buffer = PlatformAllocatePartition(MegaBytes(2));

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
        sp_rp_desc.depth_attachment.clear_depth               = 10000.0f;

        //stencil attachment description
        sp_rp_desc.stencil_attachment.description.texture     = depth_texture;
        sp_rp_desc.stencil_attachment.description.loadAction  = LoadActionClear;
        sp_rp_desc.stencil_attachment.description.storeAction = StoreActionStore;
    
        sp_rp_desc.depth_attachment = sp_rp_desc.depth_attachment;
        sp_rp_desc.stencil_attachment = sp_rp_desc.stencil_attachment;
        RendererCode::SetRenderPassDescriptor(&sp_rp_desc);
            
        RenderPassColorAttachmentDescriptor sprite_rp_ca_desc = {};
        sprite_rp_ca_desc.clear_color = float4(0.392f,0.584f,0.929f,1);//corflower blue of course
        sprite_rp_ca_desc.description.loadAction = LoadActionLoad;
        sprite_rp_ca_desc.description.storeAction = StoreActionStore;
        RenderEncoderCode::AddRenderPassColorAttachment(&sp_rp_desc,&sprite_rp_ca_desc);
        RenderEncoderCode::SetRenderPassColorAttachmentDescriptor(&sp_rp_desc,0);
        
//        RenderPass* sb_subpass = RenderPassCode::AddRenderPass(&sb_pass_buffer, no_of_sprite_pass, sizeofc,0,&sp_rp_desc);
        default_render_pass_descriptor = sp_rp_desc;
            

        RenderMaterial material = SpriteBatchCode::CreateSpriteBatchMaterials("spritebatch_vs_matrix_indexed","spritebatch_fs_single_sampler","Test matrix buffer");

        default_pipeline_state = material.pipeline_state;
        for(int i = 0;i < buffer_count;++i)
        {
            uint32_t size = (uint32_t)buffer_size;
            buffer[i] = RenderGPUMemory::NewBufferWithLength(size,ResourceStorageModeShared);
            Assert(buffer[i].buffer);
            arena[i] = AllocatePartition(size,buffer[i].data);

            //NOTE(RAY):Same of max sprites for this sprite batch perhaps we should just take and set the max number of sprites
            //to be used per batch or capacity as another way to put it.
            uint32_t atlas_index_buffer_size = (size / SIZE_OF_SPRITE_IN_BYTES) * sizeof(uint32_t);
            atlas_index_buffer[i] = RenderGPUMemory::NewBufferWithLength(atlas_index_buffer_size,ResourceStorageModeShared);
            Assert(atlas_index_buffer[i].buffer);
            atlas_index_arena[i] = AllocatePartition(atlas_index_buffer_size,atlas_index_buffer[i].data);
            matrix_variable_size_arena[i] = AllocatePartition(atlas_index_buffer_size,matrix_variable_size_buffer[i].data);
        }

        {
            uint32_t size = (uint32_t)buffer_size;
            matrix_buffer = RenderGPUMemory::NewBufferWithLength(size,ResourceStorageModeShared);
            Assert(matrix_buffer.buffer);
            matrix_buffer_arena = AllocatePartition(size,matrix_buffer.data);
        }
        
        // YoyoSpriteBatchRenderer::InitSize(&pr_sb_buffer, RendererCode::dim, 1024 * SIZE_OF_SPRITE_IN_BYTES, LoadActionLoad,false,true,false,true);
//////       // prev_frame_pipeline_state = pr_sb_buffer.sb.material.pipeline_state;
//////       // default_pipeline_state = pr_sb_buffer.sb.material.pipeline_state;
    }

    void Init()
    {
        RenderCache::Init(MAX_PSO_STATES);
        
        glemu_tex_id = 0;
        default_buffer_size = MegaBytes(1);
        buffer_count = 3;
        buffer_size = 1024 * SIZE_OF_SPRITE_IN_BYTES;
        temp_deleted_tex_entries = YoyoInitVector(1,GLTextureKey,false);
        
        AnythingRenderSamplerStateCache::Init(4096);
        AnythingCacheCode::Init(&buffercache,4096,sizeof(TripleGPUBuffer),sizeof(uint64_t));
        AnythingCacheCode::Init(&programcache,4096,sizeof(GLProgram),sizeof(GLProgramKey));
        AnythingCacheCode::Init(&cpubuffercache,4096,sizeof(CPUBuffer),sizeof(uint64_t));
        AnythingCacheCode::Init(&depth_stencil_state_cache,4096,sizeof(DepthStencilState),sizeof(DepthStencilDescription));
        AnythingCacheCode::Init(&gl_texturecache,4096,sizeof(GLTexture),sizeof(GLTextureKey),true);

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
        AnythingCacheCode::Init(&resource_managment_tables.released_textures_table,4096,sizeof(ReleasedTextureEntry),sizeof(GLTextureKey),true);
        
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
        if(texture->id == 15)
        {
            int a = 0;   
        }
        if(texture->id== 2 || texture->id == 119)
        {
            int a = 0;
        }
        GLTextureKey ttk = {};
        //ttk.api_internal_ptr = texture->texture.state;
        ttk.format = texture->texture.descriptor.pixelFormat;
        ttk.width = texture->texture.descriptor.width;
        ttk.height = texture->texture.descriptor.height;
        ttk.sample_count = texture->texture.descriptor.sampleCount;
        ttk.storage_mode = texture->texture.descriptor.storageMode;
        ttk.allowGPUOptimizedContents = texture->texture.descriptor.allowGPUOptimizedContents;
        ttk.gl_tex_id = texture->id;
//Should we allow textures that are not in the cache to be deleted and
        //TODO(Ray):Found out why some textures are no in the cache.
//If we are not in the texturecache cant delete it since its not a texture we know about.
        if(AnythingCacheCode::DoesThingExist(&gl_texturecache,&ttk))
        {
            if(!AnythingCacheCode::DoesThingExist(&resource_managment_tables.released_textures_table,&ttk))
            {
                GLTexture* tex = GetThingPtr(&gl_texturecache,&ttk,GLTexture);
                Assert(!texture->texture.is_released);
                if(!tex->texture.is_released)
                {
                    Assert(!tex->texture.is_released);
                    if(tex->texture.state != texture->texture.state)
                    {
                   
                        GLTexture* tex = GetThingPtr(&gl_texturecache,&ttk,GLTexture);
                        int a =0;
                    }
            
                    ReleasedTextureEntry rte = {};
                    rte.tex_key = ttk;
                    rte.delete_count = GLEMU_DEFAULT_TEXTURE_DELETE_COUNT;
                    rte.current_count = 0;
                    rte.thread_id = YoyoGetThreadID();
                    rte.is_free = false;
                    {
                        tex->texture.is_released = true;
                        AnythingCacheCode::AddThingFL(&resource_managment_tables.released_textures_table,&ttk,&rte);
                    }                                    
                }
            }
        }
        EndTicketMutex(&texture_mutex);
    }
    
    //NOTE(Ray)IMPORTANT:This must be used in a thread safe only section of code
    static uint64_t GLEMuGetNextTextureID()
    {
        return ++glemu_tex_id;
    }
    
    bool GLIsValidTexture(GLTexture texture)
    {
        bool result = false;
        if(texture.id == 15)
        {
            int a = 0;
        }
        GLTextureKey ttk = {};
//        ttk.api_internal_ptr = texture.texture.state;
        ttk.format = texture.texture.descriptor.pixelFormat;
        ttk.width = texture.texture.descriptor.width;
        ttk.height = texture.texture.descriptor.height;
        ttk.sample_count = texture.texture.descriptor.sampleCount;
        ttk.storage_mode = texture.texture.descriptor.storageMode;
        ttk.allowGPUOptimizedContents = texture.texture.descriptor.allowGPUOptimizedContents;
        ttk.gl_tex_id = texture.id;
#if 1
        if(!AnythingCacheCode::DoesThingExist(&gl_texturecache,&ttk))
        {
            result = false;
        }
        else if(!AnythingCacheCode::DoesThingExist(&resource_managment_tables.released_textures_table,&ttk))
        {
            result = true;
        }



#else
        if(!AnythingCacheCode::DoesThingExist(&resource_managment_tables.released_textures_table,&ttk))
        {
            result = true;
        }
#endif
        return result;        
    }

    //TODO(Ray):Add a list that has the list of iterable / non free entries in the anythings
    //so we can iterate over just the current non free entries in the list. rather than what we are doing now
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
                if(rte->tex_key.gl_tex_id == 15)
                {
                    int a = 0;
                }
                //(ReleasedTextureEntry*)resource_managment_tables.released_textures_table.anythings.base + i;
                //NOTE(Ray):Could probably put this in a hash but the assumption is that you wont do this often and surely not on purpose. ;)
                //We could also have a define where you could assert here. or when you try to use a texture that is already deleted.
                //NOTE(Ray):Now we are trying to just delete all textures at the endo fthe frame or a lil after because I think tha twill be enough
                //for VGR
                ++rte->current_count;
                if(rte->current_count >= rte->delete_count  && rte->is_free == false)
                {
                    if(AnythingCacheCode::DoesThingExist(&gl_texturecache,&rte->tex_key))
                    {
                        GLTexture* tex = GetThingPtr(&gl_texturecache,&rte->tex_key,GLTexture);
                           if(tex->id != rte->tex_key.gl_tex_id)
                           {
                               int a  = 0;
                               tex = GetThingPtr(&gl_texturecache,&rte->tex_key,GLTexture);
                           }
                        Assert(tex->id == rte->tex_key.gl_tex_id);
                        Assert(tex->texture.state);
                        Assert(tex->texture.is_released);
                        {
                            RendererCode::ReleaseTexture(&tex->texture);
                        }
                    }
                    else
                    {
                        Assert(false);
                    }
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
            if(tkey->gl_tex_id == 2 || tkey->gl_tex_id == 119)
            {
                int a = 0;
            }
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
        Assert(result);
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
    //TODO(Ray):Probably this is a bad idea 
    SamplerDescriptor GetSamplerDescriptor()
    {
        return samplerdescriptor;        
    }

    //TODO(Ray):Rename this later
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
            Assert(buffer.buffer[i].buffer);
            buffer.arena[i] = AllocatePartition(size,buffer.buffer[i].data);
        }
        uint64_t tt = bindkey;
        AnythingCacheCode::AddThing(&buffercache,(void*)&tt,&buffer);
    }

    //Add a key to the list pointing to the buffer that is previously allocated.
    //if the key is a duplicate no need to add it.
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
 //      Assert(!texture.texture.is_released);
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

/*
  if(texture.is_released)
  {
  UsedButReleasedEntry e = {};
  e.texture_key = texture.key;
  YoyoPushBack(&resource_managment_tables.used_but_released_table,e);
  }
*/
        
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
//        if(AnythingCacheCode::DoesThingExistByKey(&gl_texturecache,texture.key))
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

        /*
          GLTexture* texptr = GetThingByKeyPtr(&gl_texturecache,texture.key,GLTexture);
          UseTexture(texptr);
        */
            
        YoyoStretchPushBack(&currently_bound_frag_textures,entry);
        float2 start_count = range_of_current_bound_frag_textures;
        start_count += float2(0,1); 
        range_of_current_bound_frag_textures = start_count;
    }

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
    
    void AddBufferBinding(GPUBuffer buffer,uint64_t index,uint64_t offset)
    {
        Assert(buffer.buffer);
        BufferBindingTableEntry entry = {};
        entry.buffer = buffer;
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
        uint64_t tt = bindkey;
        return GetThingPtr(&cpubuffercache,(void*)&tt,CPUBuffer);        
    }

    YoyoVector GetCPUBufferList()
    {
        return cpubuffercache.anythings;
    }

    UniformBindResult AddUniformDataAtBinding(uint64_t bindkey,void* uniform_data,memory_index size)
    {
        UniformBindResult result;
//Assert(size < Kilobytes(2))
        CPUBuffer* buf = GetCPUBufferAtBinding(bindkey);
        float2 oldft = float2(0.0f);

        float2* ft = YoyoPeekVectorElement(float2,&buf->ranges);
        if(ft)
        {
            oldft = *ft;
        }
        
        float2 newft = float2(oldft.y(),oldft.y() + size);
        YoyoStretchPushBack(&buf->ranges,newft);

//        float2 getf = *YoyoPeekVectorElement(float2, &buf->ranges);
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
    //NOTE(Ray):Will try to emulate teh opengl api a lil closer
    //but dont make it better than the real opengl? lets see
    //ahhhnever mind no time for that shit... 
//    GLuint glCreateShader(	GLenum shaderType);    
//    void glShaderSource(GLuint shader,GLsizei count,const GLchar **string,const GLint *length);
//    void glCompileShader(GLuint shader);
    GLProgram GetDefaultProgram()
    {
        return default_program;
    }
    
    //NOTE(Ray):TODO(Ray):What would be nice is if we had some introspection into the shader and could get
    //out and build the vertex description from that.
    //Than use that to also drive what buffers we will need etc... for simpler and faster iteration times.
    GLProgram AddProgramFromMainLibrary(const char* vs_name,const char* fs_name,VertexDescriptor vd)
    {
        RenderShader s = {};
        RenderShaderCode::InitShaderFromDefaultLib(&s,vs_name,fs_name);
        GLProgram result;
        result.shader = s;
        result.vd = vd;
        result.last_fragment_buffer_binding = uniform_buffer_bindkey;
        result.last_fragment_data_index = 0;
        result.last_vertex_buffer_binding = uniform_buffer_bindkey;
        result.last_vertex_data_index = 0;
        //TODO(Ray):Should return the key?
        GLProgramKey program_hash_key = {(uint64_t)s.vs_object,(uint64_t)s.ps_object};
        AnythingCacheCode::AddThing(&programcache,(void*)&program_hash_key,&result);
        GLProgram* p = GetProgramPtr(program_hash_key);
//        p->id = hash_key;
//        result.id = hash_key;
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

/*
    GLProgram* GetProgramPtrByID(uint64_t key)
    {
        return GetThingByKeyPtr(&programcache,key,GLProgram);
    }
*/
    
    uint32_t GetDepthStencilStateCount()
    {
        return depth_stencil_state_cache.anythings.count;
    }

    //NOTE(Ray):This is very not good at the moment
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
        //Assert(last.size == size);
        return AddUniformDataAtBinding(buffer_binding,last.ptr,size);
    }
    
    GLTexture TexImage2D(void* texels,float2 dim,PixelFormat format,SamplerDescriptor sd,TextureUsage usage)
    {
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
            RenderGPUMemory::ReplaceRegion(texture.texture,region,0,texels,4 * dim.x());
        }

        BeginTicketMutex(&texture_mutex);

        texture.gen_thread = YoyoGetThreadID();
        texture.id = GLEMuGetNextTextureID();
        
        GLTextureKey k = {};
//        k.api_internal_ptr = texture.texture.state;
        k.format = texture.texture.descriptor.pixelFormat;
        k.width = texture.texture.descriptor.width;
        k.height = texture.texture.descriptor.height;
        k.sample_count = texture.texture.descriptor.sampleCount;
        k.storage_mode = texture.texture.descriptor.storageMode;
        k.allowGPUOptimizedContents = texture.texture.descriptor.allowGPUOptimizedContents;
        k.gl_tex_id = texture.id;

#if 0
        if(AnythingCacheCode::DoesThingExist(&gl_texturecache,&k))
        {
            AnythingCacheCode::DoesThingExist(&gl_texturecache,&k);
            GLTexture* t = GetThingPtr(&gl_texturecache,&k,GLTexture);
            AnythingCacheCode::AddThingFL(&gl_texturecache,&k,&texture);
            Assert(false);
        }
#endif

        texture.texture.is_released = false;
        if(texture.id == 25 || texture.id == 15 || texture.id == 4)
        {
            int a = 0;
        }
        if(texture.id == 2 || texture.id == 119)
        {
            int a = 0;
        }
        if(!AnythingCacheCode::AddThingFL(&gl_texturecache,&k,&texture))
        {
            PlatformOutput(true,"Texture already Exist was not added to texture cache");
        }

#if 0
        GLTexture* t = GetThingPtr(&gl_texturecache,&k,GLTexture);
//        if(t->texture.state != k.api_internal_ptr)
        {
            int a = 0;
        }
        t = GetThingPtr(&gl_texturecache,&k,GLTexture);
        Assert(t->texture.id == k.gl_tex_id);
#endif

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
    //TODO(Ray):Ensure we never try to add a uniform state that is larget than 2kb
    //NOTE(Ray):Unlike gl We provide the last known state of the uniform data to the user so they can manipulate
    //it how they see fit at each state more explicitely.
    void* SetUniformsFragment_(memory_index size)
    {
        Assert(size <= KiloBytes(4));
        //GLProgramKey key = {current_program.shader,current_program.vd};
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

    
    //NOTE(Ray):No sparese entries in tables.
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
        
        ///pr_sb_buffer.draw_index++;
        //pr_sb_buffer.uniform_table_index = 0;
        Assert(index == vf_index);
        return index;
    }
    
    void EndDraw(uint32_t unit_size)
    {
//        pr_sb_buffer.last_unit_size = unit_size;
//        pr_sb_buffer.sb.texture.state = nullptr;
//        pr_sb_buffer.sb.gl_texture.texture.state = nullptr;
//        pr_sb_buffer.sb.gl_texture.sampler.state = nullptr;
//        pr_sb_buffer.sb.current_count = 0;
        current_count = 0;
//        pr_sb_buffer.sb.is_primitive_triangles = false;
        
//        pr_sb_buffer.buffer_range = float2(0.0f);
//        pr_sb_buffer.texture_buffer_range = float2(0.0f);

//        pr_sb_buffer.framebuffer_state = framebuffer_none;

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
        //YoyoClearVector(&OpenGLEmu::glemu_buffer);
        current_count = 0;
        uint32_t bi =  current_buffer_index;
        arena[bi].used = 0;
        atlas_index_arena[bi].used = 0;
        matrix_buffer_arena.used = 0;
//        pr_sb_buffer.sb.from_to = float2(0.0f);
//        pr_sb_buffer.sb.from_to_bytes = float2(0.0f);
//        pr_sb_buffer.sb.from_to_matrix_index_bytes = float2(0.0f);
//        pr_sb_buffer.draw_index = 0;
//        pr_sb_buffer.buffer_range = float2(0.0f);
//        pr_sb_buffer.texture_buffer_range = float2(0.0f);
        
        //reset all uniform buffers
        CPUBuffer* ub = OpenGLEmu::GetCPUBufferAtBinding(0);
        YoyoClearVector(&ub->ranges);
        YoyoClearVector(&ub->unit_sizes);
        ub->entry_count = 0;
        ub->buffer.used = 0;            

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
//Only move our buffers forward if the we got a signal that the buffers are donen writing writing.        
        current_buffer_index = (current_buffer_index + 1) %  buffer_count;

        CheckPurgeTextures();
    }
    
    //TODO(Ray):Should be renamed
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
        AddHeader(glemu_bufferstate_scissor_test_enable);
        GLEMUScissorTestCommand* command = AddCommand(GLEMUScissorTestCommand);
        command->is_enable = false;
    }

    void ScissorTest(int x,int y,int width,int height)
    {
        AddHeader(glemu_bufferstate_scissor_rect_change);
        GLEMUScissorRectCommand* command = AddCommand(GLEMUScissorRectCommand);
        ScissorRect s_rect;
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
    
//Depth and stencil
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
    void DrawArrays(uint32_t current_count,uint32_t unit_size)
    {
        AddHeader(glemu_bufferstate_draw_arrays);
        GLEMUDrawArraysCommand* command = AddCommand(GLEMUDrawArraysCommand);                
        command->is_from_to = true;
        command->is_primitive_triangles = false;
        command->topology = topology_triangle;

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
//TODO(Ray):Do we need the current count ?
        EndDraw(unit_size);
    }

    void DrawArrayPrimitives(uint32_t current_count,uint32_t unit_size)
    {
        AddHeader(glemu_bufferstate_draw_arrays);
        GLEMUDrawArraysCommand* command = AddCommand(GLEMUDrawArraysCommand);                
        command->is_from_to = true;
        command->is_primitive_triangles = true;
        command->current_count = current_count;
        command->topology = topology_triangle;
//        pr_sb_buffer.sb.current_count = current_count;

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
        EndDraw(unit_size);
    }
     
#define Pop(ptr,type) (type*)Pop_(ptr,sizeof(type));ptr = (uint8_t*)ptr + (sizeof(type));
   static inline void*  Pop_(void* ptr,uint32_t size)
   {
       return ptr;
   }
    
    void Execute()
    {
        void* c_buffer = RenderEncoderCode::CommandBuffer();

//define start values for the gl render state    
        ScissorRect default_s_rect = {0,0,(int)RendererCode::dim.x(),(int)RendererCode::dim.y()};
        ScissorRect s_rect_value = default_s_rect;
        Drawable current_drawable = RenderEncoderCode::GetDefaultDrawableFromView();
        
        DepthStencilDescription current_depth_desc = OpenGLEmu::GetDefaultDepthStencilDescriptor();
        //TODO(Ray):Make sure this is set.
        GLProgram current_program = {};
        GLProgram default_program = {};
        TripleGPUBuffer* default_bind_buffer = OpenGLEmu::GetBufferAtBinding(0);

        uint32_t render_encoder_count = 0;
        float4 current_clear_color = float4(0.0f);

        if(current_drawable.state)
        {
            //SpriteBatchBuffer* sbb = nullptr;//&sb_buffer;
            //Set default in_params for passes
            MatrixPassInParams in_params = {};
            in_params.s_rect = default_s_rect;
            in_params.current_drawable = current_drawable;
            in_params.viewport = float4(0,0,current_drawable.texture.descriptor.width,current_drawable.texture.descriptor.height);
            in_params.vertexbuffer = default_bind_buffer;
            in_params.pipeline_state = default_pipeline_state;

            RenderPipelineState prev_pso = {};
            RenderPassDescriptor current_pass_desc = default_render_pass_descriptor;//pr_sb_buffer.render_pass_descriptor;
            RenderPassDescriptor prev_pass_desc = {};
            RenderPassDescriptor last_set_pass_desc = {};
            Texture render_texture = current_drawable.texture;
            Texture current_render_texture = render_texture;
            
            bool init_params = false;
            bool was_a_clear_encoder = false;

            u32 current_command_index = 0;
            void* at = command_list.buffer.base;
            while (current_command_index < command_list.count)
            {
                GLEMUCommandHeader* header = (GLEMUCommandHeader*)at;
                at = (uint8_t*)at + sizeof(GLEMUCommandHeader);// Pop(at,GLEMUCommandHeader);//(GLEMUCommandHeader*)at;
                GLEMUBufferState command_type = header->type;
                 ++current_command_index;
//                switch(command_type)
                {
                    {
//                        case glemu_bufferstate_debug_signpost:(framebuffer_state == framebuffer_debug_signpost)
                        if(command_type == glemu_bufferstate_debug_signpost)
                        {
                            
#ifdef METALIZER_INSERT_DEBUGSIGNPOST
//                            RenderDebug::InsertDebugSignPost(in_params.re,sbb->string);
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
//                            PlatformOutput(debug_out_general,"Setting last set_pass_desc\n");                            
                            //Verify and set pipeline states attachments to the same as our current renderpass
                            //Do depth and stencil only for now bu tlater we want to ensure that our color attachments match as well.
                            if(in_params.pipeline_state.desc.depthAttachmentPixelFormat != current_pass_desc.depth_attachment.description.texture.descriptor.pixelFormat)
                            {
                                RenderPipelineStateDesc pd = in_params.pipeline_state.desc;
                                pd.depthAttachmentPixelFormat = current_pass_desc.depth_attachment.description.texture.descriptor.pixelFormat;
                                pd.stencilAttachmentPixelFormat = current_pass_desc.depth_attachment.description.texture.descriptor.pixelFormat;
                                // || current_pass_desc.depth_attachment.description.texture.is_released
                                //if its and someone tried to use it should just set it as a non texture
                                //and sync everything  just dont let client use invalid textures as color attachments
                                //put out a warning or silently fail?
                                if(current_pass_desc.depth_attachment.description.texture.state == nullptr)
                                {
                                    pd.depthAttachmentPixelFormat = PixelFormatInvalid;
                                    pd.stencilAttachmentPixelFormat = PixelFormatInvalid;
                                }

//                                PlatformOutput(debug_out_general,"NewPIpelineState::pd\n");                            
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
                                temp_rect_value.y = default_s_rect.y - temp_rect_value.y;
                                //NOTE(Ray):Not allowed to specify a value outside of the current renderpass width in metal.
                                int diffw = (int)current_render_texture.descriptor.width - (temp_rect_value.x + temp_rect_value.width);
                                int diffh = (int)current_render_texture.descriptor.height - (temp_rect_value.y + temp_rect_value.height);
                                if(diffw < 0)
                                    temp_rect_value.width += diffw;
                                if(diffh < 0)
                                    temp_rect_value.height += diffh;
                                //TODO(Ray):Look for a way to get rid of these checks.
                                temp_rect_value.x = clamp(temp_rect_value.x,0,current_render_texture.descriptor.width);
                                temp_rect_value.y = clamp(temp_rect_value.y,0,current_render_texture.descriptor.height);
                                temp_rect_value.width = clamp(temp_rect_value.width,0,current_render_texture.descriptor.width - temp_rect_value.x);
                                temp_rect_value.height = clamp(temp_rect_value.height,0,current_render_texture.descriptor.height - temp_rect_value.y);

//                                float final_width = clamp(,0,current_render_texture.descriptor.width);
//                                float final_height = clamp(,0,current_render_texture.descriptor.height);
                                
                                in_params.s_rect = temp_rect_value;
                                RenderEncoderCode::SetScissorRect(&in_params.re, in_params.s_rect);
                            }
                            init_params = true;
                        }
                        
                        if(command_type == glemu_bufferstate_start)
                        {
                            GLEMUFramebufferStart* command = Pop(at,GLEMUFramebufferStart);//(GLEMUFramebufferStart)at;
                            if(command->texture.state != current_render_texture.state)
                            {
                                render_texture = command->texture;

//NOTE(Ray):Since we are setting a new render target framebuffer here we default the scissor rect to thte size of the
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
//                                PlatformOutput(debug_out_general,"Framebuffer_framebuffer_end::New Pipeline State\n");
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
                                 //color pass clear
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
                            continue;
                        }

                        else if(command_type == glemu_bufferstate_clear_stencil_value)
                        {
                            GLEMUClearStencilCommand* command = Pop(at,GLEMUClearStencilCommand);
                            continue;
                        }

                        else if(command_type == glemu_bufferstate_clear_color_value)
                        {
                            GLEMUClearColorCommand* command = Pop(at,GLEMUClearColorCommand);                            
                            current_clear_color = command->clear_color;
                            continue;
                        }

                        else if(command_type == glemu_bufferstate_clear_color_and_stencil_value)
                        {
                            GLEMUClearColorAndStencilCommand* command = Pop(at,GLEMUClearColorAndStencilCommand);
                            current_clear_color = command->clear_color;
                            continue;
                        }
        
                        else if(command_type == glemu_bufferstate_viewport_change)
                        {
                            GLEMUViewportChangeCommand* command = Pop(at,GLEMUViewportChangeCommand);
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
                            //NOTE(Ray):If we actually were trying to write an opengl Emulator full on we would need to test every single attachment descriptor
                            RenderPipelineStateDesc pd = in_params.pipeline_state.desc;
                            RenderPipelineColorAttachmentDescriptor cad = in_params.pipeline_state.desc.color_attachments.i[0];
//                            if(cad.sourceRGBBlendFactor != source || cad.destinationRGBBlendFactor != dest)
                            {
                                //changeBlendFactors
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

//                                PlatformOutput(debug_out_general,"Framebuffer_blend_change::New Pipeline State\n");
                                in_params.pipeline_state = next_pso;
                                RenderEncoderCode::SetRenderPipelineState(&in_params.re,in_params.pipeline_state.state);
                            }
                            continue;
                        }
                       
                        else if(command_type == glemu_bufferstate_shader_program_change)
                        {
                            GLEMUUseProgramCommand* command = Pop(at,GLEMUUseProgramCommand);
//                            if(sbb->gl_program.id != current_program.id)
                            {
                                GLProgram new_program = command->program;
                                //                       current_vertex_buffer = OpenGLEmu::GetBufferAtBinding(0);
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
                                
//                                PlatformOutput(debug_out_general,"Framebuffer_shader_program_change::New Pipeline State\n");
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
                            RenderEncoderCode::SetScissorRect(&in_params.re, in_params.s_rect);
                            continue;
                        }
                        
                        else if(command_type == glemu_bufferstate_scissor_test_disable)
                        {
                            GLEMUScissorTestCommand* command = Pop(at,GLEMUScissorTestCommand);
                            in_params.is_s_rect = false;
                            ScissorRect new_s_rect;
                            new_s_rect.width = current_render_texture.descriptor.width;
                            new_s_rect.height = current_render_texture.descriptor.height;
                            new_s_rect.x = 0;
                            new_s_rect.y = 0;
                            //in_params.s_rect = new_s_rect;
                            RenderEncoderCode::SetScissorRect(&in_params.re, new_s_rect);
                            continue;
                        }

                        else if(command_type == glemu_bufferstate_scissor_rect_change)
                        {
                            GLEMUScissorRectCommand* command = Pop(at,GLEMUScissorRectCommand);
                            ScissorRect temp_rect_value = command->s_rect;
                            //NOTE(Ray):GL is from bottom left we are top left converting y cooridinates to match
                            temp_rect_value.y = current_render_texture.descriptor.height - (temp_rect_value.height + temp_rect_value.y);
                            //temp_rect_value.height = default_s_rect.height - temp_rect_value.height;
                            //NOTE(Ray):Not allowed to specify a value outside of the current renderpass width in metal.
                            int diffw = (int)current_render_texture.descriptor.width - (temp_rect_value.x + temp_rect_value.width);
                            int diffh = (int)current_render_texture.descriptor.height - (temp_rect_value.y + temp_rect_value.height);
                            if(diffw < 0)
                                temp_rect_value.width += diffw;
                            if(diffh < 0)
                                temp_rect_value.height += diffh;

                            //TODO(Ray):Look for a way to get rid of these checks.
                            temp_rect_value.x = clamp(temp_rect_value.x,0,current_render_texture.descriptor.width);
                            temp_rect_value.y = clamp(temp_rect_value.y,0,current_render_texture.descriptor.height);
//                            temp_rect_value.width = clamp(temp_rect_value.width,0,current_render_texture.descriptor.width);
//                            temp_rect_value.height = clamp(temp_rect_value.height,0,current_render_texture.descriptor.height);
                            temp_rect_value.width = clamp(temp_rect_value.width,0,current_render_texture.descriptor.width - temp_rect_value.x);
                            temp_rect_value.height = clamp(temp_rect_value.height,0,current_render_texture.descriptor.height - temp_rect_value.y);

//                            PlatformOutput(debug_out_general,"Scissor Rect Change\n");                            
                            s_rect_value = temp_rect_value;
                            in_params.s_rect = temp_rect_value;
                            if(in_params.is_s_rect)
                            {
                                RenderEncoderCode::SetScissorRect(&in_params.re, in_params.s_rect);                            
                            }
                            continue;
                        }

                        else if(command_type == glemu_bufferstate_stencil_enable)
                        {
                            GLEMUStencilStateCommand* command = Pop(at,GLEMUStencilStateCommand);
                            
#ifdef METALIZER_INSERT_DEBUGSIGNPOST
//                            char* string = "Stencil Enabled:";
//                            RenderDebug::InsertDebugSignPost(in_params.re,string);
#endif
//                            RenderDebug::PushDebugGroup(in_params.re,string);
//                            PlatformOutput(debug_out_general,"Framebuffer_stencil_enable\n");
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
//                                PlatformOutput(debug_out_general,"End Encoding Setting renderpassdesc to have texture\n");
                                init_params = false;                                                          
                            }
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
//                            char* string = "Stencil Disabled:";
//                            RenderDebug::InsertDebugSignPost(in_params.re,string);
#endif
                            continue;
                        }

                        else if(command_type == glemu_bufferstate_stencil_mask)
                        {
                            GLEMUStencilMaskCommand* command = Pop(at,GLEMUStencilMaskCommand);
                            current_depth_desc.frontFaceStencil.write_mask = command->write_mask_value;
                            current_depth_desc.backFaceStencil.write_mask = command->write_mask_value;
                            DepthStencilState state = OpenGLEmu::GetOrCreateDepthStencilState(current_depth_desc);
                            RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
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
                            DepthStencilState state = OpenGLEmu::GetOrCreateDepthStencilState(current_depth_desc);
                            RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
                            continue;
                        }

                        else if(command_type == framebuffer_stencil_op_sep)
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
//                                PlatformOutput(debug_out_uniforms,"UniformBinding table entry : v_size :%d :  buffer index %d \n",uni_entry.v_size,4);
                            }
                            if(uni_entry.f_size > 0)
                            {
                                RenderEncoderCode::SetFragmentBytes(&in_params.re,uni_entry.f_data,uni_entry.f_size,4);
//                                PlatformOutput(debug_out_uniforms,"UnidformBinding table entry : f_size :%d : buffer index %d \n",uni_entry.f_size,4);
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

                                BeginTicketMutex(&texture_mutex);                                
                                if(GLIsValidTexture(final_tex))
                                {
//                                    PlatformOutput(debug_out_high,"Attempt To bind invalid texture Invalid texture  \n");
                                }
                                else
                                {
                                    final_tex = default_texture;
                                }
                                EndTicketMutex(&texture_mutex);

                                Assert(!final_tex.texture.is_released);
                                RenderEncoderCode::SetFragmentTexture(&in_params.re,&final_tex.texture,entry->tex_index);
                                //TODO(Ray):Allow for mutliple sampler bindings or perhaps none and use shader defined ones
                                //for now all textures use the sampler index 0 and they must have one defined.
                                //In other words a GLTexture at teh moment means you have a sampler with you by default.
                                RenderEncoderCode::SetFragmentSamplerState(&in_params.re,&final_tex.sampler,entry->sampler_index);
//                                PlatformOutput(debug_out_high,"texture binding entry : index:%d : range index %d \n",entry->tex_index,i);
//                                PlatformOutput(debug_out_high,"sampler binding entry : index:%d : range index %d \n",entry->sampler_index,i);
                            }

                            //TODO(Ray):For every vertex or fragment texture binding add a binding if there is no
                            //binding for the shader at that index we should know right away we would need some introspection into the shader/
                            //at that point but we dont have that yet for current projects its not an issue.

                            //Buffer bindings
                            //TODO(Ray):Allow for buffer bindings on the fragment and compute function
                            float2 buffer_range = command->buffer_range;
                            for(int i = buffer_range.x();i < buffer_range.y();++i)
                            {
                                BufferBindingTableEntry* entry = YoyoGetVectorElement(BufferBindingTableEntry,&currently_bound_buffers,i);
                                RenderEncoderCode::SetVertexBuffer(&in_params.re,&entry->buffer,entry->offset,entry->index);
//                                PlatformOutput(debug_out_high,"buffer binding entry : index:%d : offset %d : range index %d \n",entry->index,entry->offset,i);
                            }

                            PlatformOutput(true, "GLEMU DrawingPrimitive.\n");
                            RenderCommandEncoder re = in_params.re;

                            uint32_t bi = current_buffer_index;
 
                            uint32_t current_count = command->current_count;
                            if(current_count > 0)
                            {
                                GPUBuffer vertexbuffer = in_params.vertexbuffer->buffer[bi];
                                RenderEncoderCode::DrawPrimitives(&re, command->topology, 0, (current_count));
                            }
                        }
                    }
                    
                }//end switch
            }

            RenderEncoderCode::AddCompletedHandler(c_buffer,[](void* arg)
                                                  {
                                                     DispatchSemaphoreT* sema = (DispatchSemaphoreT*)arg;
                                                     RenderSynchronization::DispatchSemaphoreSignal(sema);
                                                  },&pr_sb_buffer.sb.semaphore);
         
            if(init_params)
            {
                RenderEncoderCode::EndEncoding(&in_params.re);            
            }
            
            //Tell the gpu to present the drawable that we wrote to
            RenderEncoderCode::PresentDrawable(c_buffer,current_drawable.state);
            RenderEncoderCode::Commit(c_buffer);
            command_list.count = 0;
            prev_frame_pipeline_state = in_params.pipeline_state;
        }
    
        //Render stats
        uint32_t pipelinestate_count = RenderCache::GetPipelineStateCount();
        PlatformOutput(true, "Renderpipeline states: %d\n",pipelinestate_count);
        uint32_t depth_stencil_state_count = RenderCache::GetPipelineStateCount();
        PlatformOutput(true, "DepthStencilState count: %d\n",depth_stencil_state_count);
        PlatformOutput(true, "RenderEncoder count: %d\n",render_encoder_count);
        PlatformOutput(true, "Draw count: %d\n",pr_sb_buffer.draw_index);
//        CaptureManager cm = RenderDebug::GetSharedCaptureManager();
//        RenderDebug::StopCapture(cm);
    }
};

#endif

