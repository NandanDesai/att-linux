#!/bin/bash
set -e

# Working dir
WORKDIR=initramfs
BUSYBOX=/bin/busybox  # Adjust if using custom build

# Clean old
rm -rf $WORKDIR
mkdir -p $WORKDIR/{bin,sbin,etc,proc,sys,usr/bin,usr/sbin,dev,tmp}

# Copy BusyBox
cp $BUSYBOX $WORKDIR/bin/

# Create symlinks for minimal tools
ln -s busybox $WORKDIR/bin/sh
ln -s busybox $WORKDIR/bin/mount
ln -s busybox $WORKDIR/bin/ls
ln -s busybox $WORKDIR/bin/echo
ln -s busybox $WORKDIR/bin/cat

# Copy any custom executable
# cp test_exec initramfs/
# chmod +x initramfs/test_exec

# Minimal init
cat > $WORKDIR/init << 'EOF'
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
echo "✅ Hello from initramfs!"
exec /bin/sh
EOF
chmod +x $WORKDIR/init

# Create initramfs archive
cd $WORKDIR
find . | cpio -H newc -o | gzip > ../initramfs.cpio.gz
cd ..
