# IMFVideoTests
Trying to get video playback on Win32 + C++ HW decode into an OpenGL texture.  This repo contains prototype code I wrote or modified in my journey to get there.

References:

(for test 001, followed the docs)
https://docs.microsoft.com/en-us/windows/win32/api/mfmediaengine/nn-mfmediaengine-imfmediaengine

(for test 002, after failing to get a DX player working following the docs, I just downloaded this and got it running on win10 as a point of reference)
https://github.com/microsoft/VCSamples/tree/master/VC2012Samples/Windows%208%20samples/C%2B%2B/Windows%208%20app%20samples/Media%20engine%20native%20C%2B%2B%20video%20playback%20sample%20(Windows%208)

(for test 003, forked the first tutorial for DX 11 base code and then 'ported' the functionality from test 002)
https://github.com/walbourn/directx-sdk-samples

(for test 5, I looked at this...
https://github.com/halogenica/WGL_NV_DX

...and the official spec page)
https://www.khronos.org/registry/OpenGL/extensions/NV/WGL_NV_DX_interop2.txt

