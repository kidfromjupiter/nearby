# Google Nearby Sharing & Connections for Linux (Unofficial)

<img width="982" height="760" alt="image" src="https://github.com/user-attachments/assets/9533ea09-81d9-4162-b90f-e0bd8b714d1d" />



## Demo


https://github.com/user-attachments/assets/048afa1e-40a4-4351-a859-c81b642fc6e3

## What
This repo consists of the entirety of google's open source nearby library. Currently, it is seperated into 3 sections. 
- Sharing
- Connections
- Presence

    
Linux specific implementation and compatibility shims are provided for building **Sharing** and **Connections**. 
Nearby presence may or may not build. I haven't tested it.

## Why
This repo could've been a PR on the official repo. All I've done is implement the platform abstraction layer google has provided for a linux specific environment.
It's not like I haven't made a PR towards the official repo. I got tired of begging the official nearby maintainers for a PR review. So here we are. 


Moreover, all I've wanted was seamless file sharing between my android devices and my linux workstation + laptop. With this repo and the example application provided here,
it accomplishes that goal perfectly. This repo wasn't created out of any altruistic goals or out of the goodness of my heart. I had a problem. I solved it. Simple as that. 


## How
### How to use
Proper documentation is coming I swear. University's getting pretty hectic so docs is on the backburner. The wiki has some more information but it's nowhere near a proper documentation. It has some good brief
overviews and where generally everything is located if you're thinking about contributing. 

If you want any clarification on anything, feel free to open an issue. I'll get back to you ASAP.

As a consolation prize, I've indexed this project using [Deepwiki](https://deepwiki.com/kidfromjupiter/nearby). You might have strong feeling about AI use. But I feel like documenting very large codebases is a perfect usecase for such models. (They are called Large Language Models for a reason )

### How to install
*coming soon*
### How to build
*coming soon*
### How to contribute
*coming soon*

## Apologies
I may have done things in *incredibly* stupid and overcomplicated ways. It doesn't certainly help that this was the way I decided to learn C++. Blessed be my naive soul. I also do not have much experience working with such 
enormous codebases. 

If you see any such stupidities, feel free to berate me in the most shameless of manners in an issue. I look forward to learning how to do it the proper way and to improve my atrocious code quality.

## Special thanks

https://github.com/proatgram and https://github.com/vibhavp

*they were the original authors of the linux platform support [PR](https://github.com/google/nearby/pull/2098) that I have based much of this codebase upon. I am standing on the shoulders
of giants*
