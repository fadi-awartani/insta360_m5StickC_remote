# Insta360 M5StickC Remote

Original documentation by Cameron Coward
serialhobbyism.com

![PXL_20250820_214156776](https://github.com/user-attachments/assets/cb476995-21c1-4d7c-819d-0d593e1e3284)

A remote for Insta360 cameras based on the M5StickC, M5StickC Plus, and M5StickC Plus2

Tested with Insta360 X5, but should work for X3, X4, and RS.

Tested on M5StickC and M5StickC Plus2, but should with on M5StickC Plus, too.

Details at [serialhobbyism.com](https://serialhobbyism.com/open-source-diy-remote-for-insta360-cameras)

Demonstration on YouTube: https://youtube.com/shorts/_0BtUzAN8ro?feature=share

------------

Make sure to set: const char REMOTE_IDENTIFIER[4] = "A01";
in the main .ino file!

That is a unique identifier and provides interference/cross communication with multiple remotes/cameras.
