# Illumos/OmniOS notes

To build libprom on Illumos/OmniOS & friends the following packages are
required: a recent gcc (e.g. gcc13) as well as gmake, cmake, libmicrohttpd
and optionally doxygen.

If your distribution does not provide a libmicrohttpd (or any of the others
mentioned above) package, you may install pkgin (if not already done) and
fetch it via pkgin.

## OmniOS
On OmniOS one may install pkgin, fetch libmicrohttpd and build libprom
as follows:
```
mkdir ~/tmp; cd ~/tmp
# check https://pkgsrc.smartos.org/packages/SmartOS/bootstrap/ to find a recent
# bootstrap archive, download and install it. E.g.:
curl -O https://pkgsrc.smartos.org/packages/SmartOS/bootstrap/bootstrap-2025Q4-x86_64.tar.gz
pfexec tar -xzf bootstrap-2025Q4-x86_64.tar.gz -C /
echo 'export PATH=/opt/local/bin:/opt/local/sbin:$PATH' >> ~/.profile
source ~/.profile

pfexec pkgin update
pkgin search microhttpd
pfexec pkgin install libmicrohttpd

# Check out libprom and build it
git clone git@github.com:jelmd/libprom.git
cd libprom
gmake

# Install it to /opt/local
pfexec gmake install

# or create an archive
gmake tgz
```
