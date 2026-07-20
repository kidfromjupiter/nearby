<img width="1661" height="569" alt="Frame 2 (2)" src="https://github.com/user-attachments/assets/db8376a9-35d9-406f-bcb8-5581cd0dd762" />



> ## 🚧 **Under Construction** 🚧  
> This repo is actively being worked on. Things are buggy, builds may fail, documentation is non-existent, and we test in prod.
>
> **You have been warned**

---

<div align="center">
  <img src="https://github.com/user-attachments/assets/4e630d75-3484-4f9d-9657-c99a2b64ab69" alt="send" width="400" />
  <img src="https://github.com/user-attachments/assets/8d925e98-f791-4cd4-8e18-f790b9d10bb5" alt="recv" width="400" />
</div>
<p align="center" italic>
     <em>(Android captured with scrcpy)</em>
</p>

## What
This repo consists of the entirety of google's open source nearby library. Currently, it is seperated into 3 (now 2) sections. 
- Sharing
- Connections
- ~Presence~ ( *Was removed by google from their repo. RIP :(*   )

    
Linux specific implementation and compatibility shims are provided for building **Sharing** and **Connections**. 

Linux implementation of a QT-based QuickShare application is also provided as a release

## Features
- Near feature parity with QuickShare on android
- No companion apps. Works with native quickshare
- Share files over the network, wifi hotspot or bluetooth
- Fast, seamless and compatible with any android device


### How to install
This repo provides prebuilt AppImages of the Quick Share application. The only officially supported distro is Fedora 43 ( what I'm using personally ) and Ubuntu 24.04 for now. The newest ubuntu images *should* work fine
although that needs to be tested. I want to support more distros so if you encounter issues installing on your distro, please let me know. 

Download and click to run on most linux distros : )

#### Prerequisites

- `systemd`
- `NetworkManager`
- `bluez >= 5.85`

**To install the prerequisites, run this command**

Fedora : 

```bash
sudo dnf install -y \
  bluez bluez-libs bluez-libs-devel \
  sdbus-cpp sdbus-cpp-devel
```
---

## Documentation
Docs is on the backburner for now

If you want any clarification on anything, feel free to open an issue. I'll get back to you ASAP.

As a consolation prize, I've indexed this project using [Deepwiki](https://deepwiki.com/kidfromjupiter/nearby). You might have strong feeling about AI use. But I feel like documenting very large codebases is a perfect usecase for such models. (They are called Large Language Models for a reason )


**To install the actual library and headers,**

Currently there are no prebuilt shared library or headers. You'll have to build them yourself

### How to build

Best place to consult would be the Github actions and workflows. 


## KNOWN BUGS
- [ ] **Launch the app when you want to receive/send**
        Keeping the application on may interfere with bluetooth devices. I am working on fixing this. So turn the application on only **when** you want to share stuff
      
- [ ] **Bluetooth classic bandwidth**
  
    File transfer on bluetooth classic is painfully slow. Bandwidth close to 20KB/s. ~May be a regression issue after bluetooth socket refactor~. May be an issue with sending back acknowledgements. Issue is present on pre-refactor versions. ~Look into Multiplexing maybe~ Multiplexing did not fix it : (?

- [ ] **Investigate why Bluetooth connection requests pairing.**
      
  Both the L2CAP socket and Bluetooth profile should be unauthenticated.
- [ ] **Handle existing files when receiving.**
      
  Currently, files are not overwritten if they already exist. Decide whether to overwrite, rename, or skip.

- [ ] **Bluetooth Classic transfer progress issue.**
      
  When transferring Android → Linux, Android shows 100% transferred but still says `Sending...`, while Linux lags behind. Could be a bottleneck or Android-side issue.

---

### New Features

- [ ] Upstream has been slowly adding webrtc support. Should we support it?
- [ ] Support enabling receive on detection of fast initiation LE advertisement

## Special thanks

https://github.com/proatgram and https://github.com/vibhavp

*they were the original authors of the linux platform support [PR](https://github.com/google/nearby/pull/2098) that I have based much of this codebase upon. I am standing on the shoulders
of giants*
