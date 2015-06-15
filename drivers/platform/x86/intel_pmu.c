/*
 * Intel PMU device driver
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pci.h>

#include <linux/pmu/intel.h>

#define IPMU_STS		0x00
#define IPMU_CMD		0x04
#define IPMU_SSC0		0x20
#define IPMU_SSC1		0x24
#define IPMU_SSC2		0x28
#define IPMU_SSC3		0x2c

/* Bits in IPMU_STS */
#define IPMU_STS_BUSY			(1 << 8)

/* Commands */
#define IPMU_CMD_INTERACTIVE		0x2201

#define PCI_DEVICE_ID_TANGIER		0x11a1

struct intel_pmu {
	struct device *dev;
	void __iomem *regs;
	int irq;
	bool on;
};

static struct intel_pmu *pmu;

bool intel_pmu_is_available(void)
{
	return pmu && pmu->on;
}
EXPORT_SYMBOL_GPL(intel_pmu_is_available);

static bool intel_pmu_is_busy(struct intel_pmu *pmu)
{
	return !!(readl(pmu->regs + IPMU_STS) & IPMU_STS_BUSY);
}

static int intel_pmu_wait(struct intel_pmu *pmu)
{
	bool busy;
	unsigned int count = 25000;

	do {
		busy = intel_pmu_is_busy(pmu);
		if (!busy)
			return 0;
		usleep_range(10, 20);
	} while (--count);

	return -EBUSY;
}

static int intel_pmu_disable(struct intel_pmu *pmu)
{
	int ret;

	ret = intel_pmu_wait(pmu);
	if (ret)
		return ret;

	/* Turn on all devices */
	writel(0, pmu->regs + IPMU_SSC0);
	writel(0, pmu->regs + IPMU_SSC1);
	writel(0, pmu->regs + IPMU_SSC2);
	writel(0, pmu->regs + IPMU_SSC3);

	/* Send interactive command to SCU */
	writel(IPMU_CMD_INTERACTIVE, pmu->regs + IPMU_CMD);

	return 0;
}

static int intel_pmu_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int ret;

	ret = pcim_enable_device(pdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "error: could not enable device\n");
		return ret;
	}

	ret = pcim_iomap_regions(pdev, 1 << 0, pci_name(pdev));
	if (ret) {
		dev_err(&pdev->dev, "I/O memory remapping failed\n");
		return ret;
	}

	pmu = devm_kzalloc(&pdev->dev, sizeof(*pmu), GFP_KERNEL);
	if (!pmu)
		return -ENOMEM;

	pmu->dev = &pdev->dev;
	pmu->regs = pcim_iomap_table(pdev)[0];
	pmu->irq = pdev->irq;

	ret = intel_pmu_disable(pmu);
	if (ret)
		return ret;

	pmu->on = true;

	pci_set_drvdata(pdev, pmu);
	return 0;
}

static const struct pci_device_id intel_pmu_pci_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_TANGIER) },
	{},
};
MODULE_DEVICE_TABLE(pci, intel_pmu_pci_ids);

static struct pci_driver ipmu_pci_driver = {
	.name		= "intel_pmu_pci",
	.probe		= intel_pmu_probe,
	.id_table	= intel_pmu_pci_ids,
};

module_pci_driver(ipmu_pci_driver);

MODULE_AUTHOR("Andy Shevchenko <andriy.shevchenko@linux.intel.com>");
MODULE_DESCRIPTION("Driver for Intel PMU device");
MODULE_LICENSE("GPL v2");
