#ifndef WEBOUT_CWEBOSD_H
#define WEBOUT_CWEBOSD_H

#include <vdr/osd.h>
#include "osd.h"
#include "global.h"

const int OSDPROVIDER_IDX = 1;

class cWebOsdProvider : public cOsdProvider {
protected:
    cOsd *CreateOsd(int Left, int Top, uint Level) override;
    bool ProvidesTrueColor() override { return true; }

public:
    cWebOsdProvider(int idx);
    ~cWebOsdProvider() override;
};

class cWebOsd : public cOsd {
private:
    friend class cWebOsdProvider;

protected:
    cWebOsd(cWebOsdProvider& Provider, int Left, int Top, uint Level);

public:
    ~cWebOsd() override;
    void Flush() override;
};

#endif // WEBOUT_CWEBOSD_H
