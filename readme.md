# Welcome to GLEMU

The aim of this project is to provide an easy to use api for using legacy projects on newer apis.
It is built on top of metalizer which is the multi platform part GLemu just provides a more abstracted "better" than opengl
renderering api. 
The main mission is to convert easily projects off of legacy graphics apis while still maintaining control for performance reasons.
There is a possiblity however that the usage could expand beyond that if it seems reasonable to do so.

##  Objectives:
### Short term: Create an opengl like rendering api that can convert legacy opengl to metal output mainly.

    1. perf not primary for short term as we mainly targeting 2d games at first.
    2. correct output will be the main focus
    3. adherance to the opengl standard is not primary. Improve what can be if possible make a more reasonable gl.
       in other words we dont need to stick to the opengl spec just take the good parts
       where it makes sense for converting projects.

### Long  term: Create a deffered rendering api that is able to create relatively efficient multithreaded rendering calls and validation
    based on a post built frame graph.

    1. Perf will be cricial in this stage and strategies to advance in that area will be extreme.
    2. At this point we will be adding completely new concepts and try to mix and match where it makes sense.

Everything here is still WIP... for release info there will be a release notes doc.

If you aquired this project as a maintainer every major release should be named.
Look here for release names to use 
First release name is Mercury(unreleased)

OpenGLEMU.h is a single file header file but requires project Metallizer project to run and render.
Use: add include header to one of the or the first file in your project to use GLEmu.
Above the include statement define GLEMU_IMPL

## !!!!!!!!!!!!!!!!!!!!!!!!!!READ FIRST!!!!!!!!!!!!!!!!!!!!!!!!!
Notes, caveats and preamble:
Short version: This lib may or may not (more likely not)make your opengl code go faster and you need to know what your
doing to get fast results.

Long version:
Things to keep in mind when using this api.
If you try to use this as you would regular opengl there will be issues you be well off to learn about the newer api's
and understand what causes pipeline states to be generated and you can avoid many pitfalls.
The main advantage is you avoid a lot of the boiler plate that comes with the newer api's but at a cost.
You can avoid many of the pitfalls by constructiong your GLCalls by batching as much as possible your draw calls.
But thats up to you based on your usage needs. Just something to keep in mind that if you do your rendering in a highly
ineffecient order there is alil that we can do to mitagate that.  It all comes down to keep things that cause state tranisitions
to a minimum and ordering your draw calls in a sane manner.   There are plans to do something like this in the backend but
is still a micro optimization and will not save you if you order things in an ineffecient manner.

There is still a lot missing and highly ineffecient.
What is included so far.
    1. Very Basic rendering and binding.
Nothing else!
No compute no tesselation no nothing at the moment.

So why shouldnt I use moltenGL?
Well maybe you should.
The advantages here are we are not neccessarily tied to a backend(someday).
Not tied to a spec.
"Simpler to use"?
Can provide a callback to write your own specific "gldriver" or modify the original one.
Able to experiment with our own possible better api that might operate at a slightly better level of abstraction(opinionated).
For quickly iterating / deffering calls , analysing the frame and ordering calls in a way that might be more effecient.
If all that doesnt interest you than you should use moltenGL if you have the budget of course.
