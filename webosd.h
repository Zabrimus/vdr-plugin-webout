#ifndef WEBOUT_CWEBOSD_H
#define WEBOUT_CWEBOSD_H

#include <vdr/osd.h>
#include "webosd.h"
#include "global.h"

class cWebOsdProvider : public cOsdProvider {
protected:
    cOsd *CreateOsd(int Left, int Top, uint Level) override;
    bool ProvidesTrueColor() override { return true; }

public:
    cWebOsdProvider();
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
