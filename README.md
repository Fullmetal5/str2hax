# str2hax

Just to get this out of the way: THIS DOES NOT WORK ON THE MINI

Alright, so with Nintendo shutting down the E-Shop there won't be a way of getting the Internet Channel anymore which means no more FlashHax.
So we need another exploit that works without sd cards and now only works with whatever default channels are installed on the Wii.
So what's the attack surface for the default channels?
Well luckily for us Nintendo decided to have their EULA for the Wii be updatable, and they decided to do this by making the EULA view actually just be Opera pointed at the page "http://cfh.wapp.wii.com/eula/XXX/YY.html".
Where XXX is the country code and YY is the language.
And since they get the page over http that means if we change the dns servers then we can switch the page out for whatever we want.
See below for the write-up.

How to setup str2hax:
1. Go to the Wii's settings the under Internet select Connection Settings and choose your currently active connection.
2. Select Change settings and scroll to the right until you get to Auto-Obtain DNS
3. Select No then select Advanced Settings.
4. Change the Primary DNS to 97.74.103.14 and the Secondary DNS to 173.201.71.14.
5. Select Confirm and then Save, you will be told you must run a connection test. (Select No to the system update prompt)
If the connection test doesn't work try running it one more time and if it still fails leave a post about it. (Please make sure you have a working internet connection in the first place.)
6. Back out to the Internet panel and choose User Agreements. Select Yes to the question about the Wii Shop Channel/WiiConnect24.
7. You will be taken to a screen telling you to review the User Agreements for the Wii. Select Next.
If you see a pony on screen telling you to wait then you have done everything correctly. The exploit takes 1-2 minutes (1:25 is usually how long mine takes), if it takes longer than 2 minutes then it probably failed. Just turn off your Wii and start again from step 6.

After a minute or two you should be booted into the HackMii Installer. If the Wii freezes on a with a bunch of white text on it please take a LEGIBLE picture of the screen. I can't help you if I can't read it.

If you got some use out of this and want to throw me some money you can do so here. (College is expensive, so is IDA)

So how does str2hax work?

Well, it's actually CVE-2009-0689 as you may have guessed if you looked at the source from the page and found this "parseFloat" as the last function call.
So what exactly is CVE-2009-0689?
It's a heap based buffer overflow that happens when attempting to convert large ascii decimals to IEEE754 decimals.
This occurs in the popular dtoa.c library by David M. Gay.
The Opera team made some changes to it but a close match for it can be found here (https://websvn.kde.org/branches/KDE/4.3/kdelibs/kjs/dtoa.cpp?revision=986143&view=markup)

With the source code in hand we can get a much better idea of what's happening here.
First we need to know why things crash when we pass in large ascii decimals, but first we need to explain how dtoa manages its memory.
And yes, dtoa is the one managing its memory.
dtoa needs to expand the ascii decimal given to its full value to be able to perform the necessary math and ensure correctness.
To do this it allocs large structs call Bigints, these blocks are stored in a linked list based on the size of the decimal they can hold. (The sizes are in powers of 2)
The format of a Bigint is given on line 463 and looks like this:

struct Bigint {
struct Bigint *next;
int k, maxwds, sign, wds;
ULong x[1];
};

You can see the next variable which will be a pointer to the next free Bigint in the list when not in use, the k variable which indicates how big of a number can be stored in it, the maxwds, sign, and wds which are used to keep track of how many words of data represent the number as well as its sign, and finally the actual number follows in the array x.
So now we know how numbers are stored we need to see how these bigints are kept when not in use.
You can see on line 471 that an array of bigints is declared called freelist and is given a size of Kmax+1, Kmax is defined to be 15 here so that means there are 16 slots for bigints to occupy.
Each index in the array points to the first bigint in the linked list of free bigints of size K.


Kmax is used to indicate what the maximum K value a number can be before the library can't handle it.
When getting these bigint dtoa uses a wrapper function called Bmalloc.
Bmalloc takes an int k and checks to see if there are any free bigints in the freelist of size k by simply checking freelist[k] for a pointer and if not it allocs one with malloc as normal.

So what happens if we pass in a number that results in K being above Kmax?
Well, things go wrong.
When you index past the end of an array you will get whatever is in memory directly after it.
Luckily for us freelist is stored in the bss section since it's a global variable and therefore the next global variable is probably what the compiler decided to put after freelist.
This turns out to be correct and the next global variable in dtoa is the p5s array which is used by pow5mult.
So what is in p5s? It's more pointers to bigint actually. But very critically, it's pointers to bigints of a much smaller size (2 in this case).
So, when asked for a bigint of size Kmax+1, instead you get back a bigint of size 2. Oops

Unfortunately dtoa is quite complicated so I've only traced a small step of events that lead to the buffer overflow.
When attempting to diff two large bigints diff tries to allocate a return bigint that will be passed back. This return bigint attempts to allocate a K of size 17 which is one above the maximum and thus accidentally grabs from p5s which at the time has a bigint of size 2 sitting in it.
diff then effectively copies ~50K of the decimal passed to it into the smaller bigint. (Actually it is a diff of our original decimal and a small decimal I can't seem to figure out's meaning.) (Also it copies much more than 50K but that's all that's usable before it starts hitting the smaller decimal and things stop being predictable.)
So that means that ~50K of our decimal ends up smashing threw the much small bigint (only allocated to something like 0x20 bytes) and we can control all 50K.

So now we need to figure out what to overwrite with our 50K of data.
Unfortunately it's to big of an overwrite to ever return back to javascript (you might be able to but I choose not to), so instead I decided to overwrite another bigint and use it's k value to get an arbitrary write off.
When dtoa wants to "free" a bigint it calls a function Bfree which actually never free's it but instead adds it to the linked list we saw above. This is how bigints are recycled.
To do this it attempts to write a pointer to the bigint to freelist[v->k] which means that it will take K from the bigint, multiply it by 4, then add it to the memory location of freelist and write a pointer to the bigint there.
Unfortunately it also writes the current value at that address as the next pointer in the bigint which happens to be the first value.
To make matters worse the next field after Next happens to be the K value which means our K value will be tied to the Next value written.
This becomes very annoying later.

So since we control the k value of the bigint, we can direct the write to any 4 byte aligned address. Luckly for us this version of Opera doesn't appear to use and aslr/dep so I choose to overwrite a return address on the stack with it.
This results in the Bigint struct being jumped to and it's fields being interpreted as code.
As you may have guessed that means we need to find a return address that not only has stored in it a valid powerpc instruction but whose address results in a K that is also valid since we won't get code execution until both fields have been executed first.
Very luckily just 2 layers deep in the stack a return address happens to decode to a valid load instruction with a register that is a valid address at return time and has a K value that is safe. This means we will safely pass over it.

Here is a more visual representation of the bigint struct we overwrite.

uint32_t chain_layout[] = {
marker, // Beginning of bigint x region
0x00000000, // End of bigint x region
0x00000000, // Heap padding
0x00000000, // Heap padding
heap_header, // Heap header for next bigint block
bigint_next, // next pointer for the following bigint
bigint_k, // k size of the following bigint
relative_jump, // A relative jump forward (overlaps maxwds)
0x00000000, // sign
0x00000000 // wds
};

With all this done we can now just insert a relative instruction in the maxwds field safely slide down the chain of jump instructions (several fake bigints were created incase we don't align things probably with heap feng shui) until we hit our payload.

In this case the payload is savezelda that has had SD/gecko support removed and really just servers as a egg hunter for a bigger payload downloaded in memory.
Due to browser memory restrictions the whole page can't be very big (~512K) and we already take up white a big with the ascii decimal.
To workarounds this the payload is DEFLATE'd and uncompressed by savezelda with a very small deflate implementation by Rich Geldreich (Thank you VERY much).

The payload that str2hax currently boots is a small network loader that download the HackMii Installer and launches it.
Random thingy: Also I didn't disable the framebuffer so you get a nice view into ram while HackMii is booting which is why you see all the glitchy green mess

How Do I Build It:
0. Grab the source code from here: https://github.com/Fullmetal5/str2hax
1. Copy your payload (MUST BE VERY SMALL, you should probably just use the network loader) into the payload folder
2. Run ./make_is.sh YOUR_PAYLOAD.elf
This will give you a payload.png
3. Run ./create.sh in the main directory
4. It will create a site.zip file with everything you need. (Apache is required for the redirects)
5. To test this you will need to setup a dns server (dnsmasq works great) and redirect cfh.wapp.wii.com to your site.
