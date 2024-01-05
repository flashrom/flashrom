FROM almalinux:9-minimal as base

RUN microdnf update -y && microdnf install -y dnf epel-release almalinux-release-devel

FROM base as flashrom
COPY build-alma-rpms.sh flashrom.spec .github/
RUN .github/build-alma-rpms.sh

FROM base
COPY --from=flashrom /root/rpmbuild/RPMS/x86_64/flashrom*.rpm /tmp
RUN dnf install -y /tmp/flashrom*.rpm
