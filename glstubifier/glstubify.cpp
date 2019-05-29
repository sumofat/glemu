#define YOYOIMPL 1
#include "../../renderer/native_graphics_api_dx12.cpp"
#include "../../RendererInclude.h"

enum GLHeaderDataBlockType
{
    glheader_data_func_sig,
    glheader_data_func_impl
};

struct GLHeaderDataBlock
{
    GLHeaderDataBlockType type;
    YoyoVector tokens;
};

struct GLHeaderData
{
    YoyoVector header_data_block;
};

GLHeaderData ParseGLHeader(MemoryArena* arena,char* text_string)
{
    uint32_t file_size_in_ascii_char = 1000;
    GLHeaderData result = {};
    result.header_data_block = YoyoInitVector(1,GLHeaderDataBlock,false);
    Tokenizer tokenizer = {};
    tokenizer.At = text_string;
    YoyoVector tokens = YoyoInitVector(1, Token,false);
    bool is_parsing = true;
    bool is_function = false;
    Token prev_token = {};
    while (is_parsing)
    {
        Token token = GetToken(&tokenizer, arena);
        YoyoStretchPushBack(&tokens, token);
        
        if(IsCommentStart(prev_token,token))
        {
            int max_iterations_allowed = INT_MAX;
            int i = 0;
            //Begin comment block
            for(;;)
            {
                token= GetToken(&tokenizer, arena);
                if(IsCommentEnd(prev_token,token) || i > max_iterations_allowed)
                {
                    break;
                }
                prev_token = token;
                ++i;
            }
            //go to the next loop iteration since we are finished with the comment and now need the next token.
            continue;
        }
        
        if(token.Type == Token_Identifier)
        {
            if (CompareChars(token.Data.String, CreateStringFromLiteral("GL_API",arena).String) && !CompareChars(prev_token.Data.String,CreateStringFromLiteral("define",arena).String))
            {
                GLHeaderDataBlock block = {};
                //We assume a function signature here because for now we know what the data is and are not trying to be comprehensive for simplicity's sake.
                block.type = glheader_data_func_sig;
                block.tokens = YoyoInitVector(1,Token,false);
                //beggining of function definition.
                is_function = true;
                YoyoStretchPushBack(&block.tokens,token);
                for(;;)
                {
                    token = GetToken(&tokenizer,arena);
                    YoyoStretchPushBack(&block.tokens,token);
                    is_function = false;
                    if(token.Type == Token_SemiColon)
                    {
                        break;
                    }
                    prev_token = token;
                }
                YoyoStretchPushBack(&result.header_data_block,block);
                continue;
            }
        }
        
        if (token.Type == Token_EndOfStream)
        {
            break;
        }
        prev_token = token;
    }
    return result;
}

enum GLStubifyReturnTypes
{
    gl_returntype_void,
    gl_returntype_uint
};

Yostr GetReturnString(GLStubifyReturnTypes type,MemoryArena* arena)
{
    Yostr result = {};
    switch(type)
    {
        case gl_returntype_void:
        {
            result = CreateStringFromLiteral("return;\n",arena);
        }break;
        case gl_returntype_uint:
        {
            result = CreateStringFromLiteral("return 0;\n",arena);
        }break;
        default:
        {
            Assert(false);
        }
    }
    return result;
}

//1. Parse and append any unknown tokens with whitespace
//
//2. When we hit a token with "GL_API" token and than ";" semi colon store that.
//3. Insert braces 
//4. Further recurse down and find the return parameter.
//5. Add function body and return parameter to the string.
//6. We might need to parse the function parameters in the case of functions that return something in the parameters.
//7. In that case we can use a switch to zero that out.
//8. All vars will be a zero return for now.
// TODO(Ray Garner): Handle const case and add some more resturn types.
//
void main()
{
    PlatformOutput(true,"------------BEGIN GLSTUBIFY------------\n");
    StringsHandler::Init();
    MemoryArena s = StringsHandler::string_memory;
    MemoryArena sm = StringsHandler::transient_string_memory;
    uint32_t length = 0;
    uint32_t lengthext = 0;
    Yostr gl_h_output = {};
    Yostr gl_h_ext_output = {};
    
    gl_h_output.String = (char*)sm.base;
    gl_h_ext_output.String = "";
    gl_h_ext_output.Length = 0;
    //
    read_file_result gl_h_file = PlatformReadEntireFile("gl.h");
    read_file_result gl_h_ext_file = PlatformReadEntireFile("glext.h");
    
    GLHeaderData header_data = ParseGLHeader(&StringsHandler::string_memory,(char*)gl_h_file.Content);
    
    //We set the maximum size of the file which we should not be going over as a subset of that.
    MemoryArena final_out = PlatformAllocatePartition(gl_h_file.ContentSize);
    MemoryArena func_sig_temp_arena = PlatformAllocatePartition(KiloBytes(2));
    
    Token prev_token = {};
    Token return_type_token = {};
    for(int i = 0;i < header_data.header_data_block.count;++i)
    {
        GLHeaderDataBlock* block = (GLHeaderDataBlock*)header_data.header_data_block.base + i;
        for(int j = 0;j < block->tokens.count;++j)
        {
            Token* t = (Token*)block->tokens.base + j;
            if(t->Type == Token_OpenParen)
            {
                AppendStringSameFrontArena(&gl_h_output,CreateStringFromLiteral("(",&s),&sm);
                PlatformOutput(true,"(",t->Data.String);
            }
            
            else if(t->Type == Token_CloseParen)
            {
                AppendStringSameFrontArena(&gl_h_output,CreateStringFromLiteral(")",&s),&sm);
                PlatformOutput(true,")",t->Data.String);
            }
            
            else if(t->Type == Token_SemiColon)
            {
                // NOTE(Ray Garner): This is the end of the signature
                //we throw away the semicolon and add create our braces
                //and return type here.
                Yostr func_stub = CreateStringFromLiteral("{\n",&func_sig_temp_arena);
                GLStubifyReturnTypes return_type = {};
                if(CompareStringtoChar(return_type_token.Data,"void"))
                {
                    return_type = gl_returntype_void;
                }
                else if(CompareStringtoChar(return_type_token.Data,"GLuint"))
                {
                    return_type = gl_returntype_uint;
                }
                Yostr return_statement = AppendString(func_stub,GetReturnString(return_type,&func_sig_temp_arena),&func_sig_temp_arena);
                func_stub = AppendString(return_statement,CreateStringFromLiteral("}\n",&func_sig_temp_arena),&func_sig_temp_arena);
                AppendStringSameFrontArena(&gl_h_output,func_stub,&sm);
                DeAllocatePartition(&func_sig_temp_arena,false);
                PlatformOutput(true,";\n");
            }
            
            else if(t->Type == Token_Comma)
            {
                AppendStringSameFrontArena(&gl_h_output,CreateStringFromLiteral(",",&s),&sm);
                PlatformOutput(true,",");
            }
            
            else if(t->Type == Token_Identifier)
            {
                if(CompareStringtoChar(prev_token.Data,"GL_API"))
                {
                    return_type_token = *t;
                }
                AppendStringSameFrontArena(&gl_h_output,t->Data,&sm);
                PlatformOutput(true,"%s ",t->Data.String);
            }
            prev_token = *t;
        }
    }
    
    PlatformOutput(true,"FinalOut: %s\n",gl_h_output.String);
    
    //Output stubbed gl.h
    char* output = "";
    char* fn = "stubbed_gl.h";
    
    Yostr final_filename = {};
    final_filename.Length = String_GetLength_Char(fn);
    final_filename.String = fn;
    final_filename.NullTerminated = true;
    char* dir = "";
    PlatformFilePointer file{};
    Yostr final_output_path = AppendStringToChar(dir,final_filename,&sm);
    PlatformWriteMemoryToFile(&file,final_output_path.String,(void*)output,length,true,"w+");
    
    //Output stubbed glext.h
    char* output_ext = "";
    char* fn_ext = "stubbed_glext.h";
    
    Yostr final_filename_ext = {};
    final_filename_ext.Length = String_GetLength_Char(fn_ext);
    final_filename_ext.String = fn_ext;
    final_filename_ext.NullTerminated = true;
    char* dirext = "";
    
    PlatformFilePointer file_ext{};
    Yostr final_output_path_ext = AppendStringToChar(dirext,final_filename_ext,&sm);
    PlatformWriteMemoryToFile(&file_ext,final_output_path_ext.String,(void*)output_ext,lengthext,true,"w+");
    
    PlatformOutput(true,"------------END GLSTUBIFY------------\n");
}


